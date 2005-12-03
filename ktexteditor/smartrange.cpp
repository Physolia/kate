/* This file is part of the KDE libraries
   Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "smartrange.h"

#include <QStack>

#include "document.h"
#include "view.h"
#include "attribute.h"
#include "rangefeedback.h"

#include <kaction.h>
#include <kdebug.h>

using namespace KTextEditor;

SmartRange::SmartRange(SmartCursor* start, SmartCursor* end, SmartRange * parent, InsertBehaviours insertBehaviour )
  : Range(start, end)
  , m_attribute(0L)
  , m_parentRange(parent)
  , m_ownsAttribute(false)
{
  setInsertBehaviour(insertBehaviour);

  // Not calling setParentRange here...:
  // 1) subclasses are not yet constructed
  // 2) it would otherwise give the wrong impression
  if (m_parentRange)
    m_parentRange->insertChildRange(this);
}

SmartRange::~SmartRange( )
{
  deleteChildRanges();

  setParentRange(0L);
  setAttribute(0L);

  /*if (!m_deleteCursors)
  {
    // Save from deletion in the parent
    m_start = 0L;
    m_end = 0L;
  }*/
}

bool SmartRange::confineToRange(const Range& range)
{
  if (!Range::confineToRange(range))
    // Don't need to check if children should be confined, they already are
    return false;

  foreach (SmartRange* child, m_childRanges)
    child->confineToRange(*this);

  return true;
}

bool SmartRange::expandToRange(const Range& range)
{
  if (!Range::expandToRange(range))
    // Don't need to check if parents should be expanded, they already are
    return false;

  if (parentRange())
    parentRange()->expandToRange(*this);

  return true;
}

void SmartRange::setRange(const Range& range)
{
  if (range == *this)
    return;

  Range old = *this;

  Range::setRange(range);

  rangeChanged(0L, old);
}

const QList<SmartRange*>& SmartRange::childRanges() const
{
  return m_childRanges;
}

SmartRange * SmartRange::childBefore( const SmartRange * range ) const
{
  int index = m_childRanges.indexOf(const_cast<SmartRange*>(range));
  if (--index >= 0)
    return m_childRanges[index];
  return 0L;
}

SmartRange * SmartRange::childAfter( const SmartRange * range ) const
{
  int index = m_childRanges.indexOf(const_cast<SmartRange*>(range));
  if (index != -1 && ++index < m_childRanges.count())
    return m_childRanges[index];
  return 0L;
}

void SmartRange::insertChildRange( SmartRange * newChild )
{
  // This function is backwards because it's most likely the new child will go onto the end
  // of the child list
  Q_ASSERT(newChild->parentRange() == this);

  // A new child has been added, so expand this range if required.
  expandToRange(*newChild);

  QMutableListIterator<SmartRange*> it = m_childRanges;
  it.toBack();

  while (it.hasPrevious()) {
    if (it.peekPrevious()->end() <= newChild->start()) {
      it.insert(newChild);
      if (it.hasNext() && it.peekNext()->start() < newChild->end())
          it.peekNext()->start() = newChild->end();
      return;
    }

    it.previous();
  }

  m_childRanges.prepend(newChild);
}

void SmartRange::removeChildRange(SmartRange* newChild)
{
  int index = m_childRanges.lastIndexOf(newChild);
  if (index != -1)
    m_childRanges.removeAt(index);
}

SmartRange * SmartRange::mostSpecificRange( const Range & input ) const
{
  if (!input.isValid())
    return 0L;

  if (contains(input)) {
    foreach (SmartRange* r, m_childRanges)
      if (r->contains(input))
        return r->mostSpecificRange(input);

    return const_cast<SmartRange*>(this);

  } else if (parentRange()) {
    return parentRange()->mostSpecificRange(input);

  } else {
    return 0L;
  }
}

SmartRange * SmartRange::firstRangeContaining( const Cursor & pos ) const
{
  if (!pos.isValid())
    return 0L;

  if (contains(pos)) {
    if (parentRange() && parentRange()->contains(pos))
      return parentRange()->firstRangeContaining(pos);

    return const_cast<SmartRange*>(this);

  } else {
    if (!parentRange())
      return 0L;

    return parentRange()->firstRangeContaining(pos);
  }
}

SmartRange * SmartRange::deepestRangeContaining( const Cursor & pos, QStack<SmartRange*>* rangesEntered, QStack<SmartRange*>* rangesExited ) const
{
  if (!pos.isValid())
    return 0L;

  return deepestRangeContainingInternal(pos, rangesEntered, rangesExited, true);
}

SmartRange * SmartRange::deepestRangeContainingInternal( const Cursor & pos, QStack<SmartRange*>* rangesEntered, QStack<SmartRange*>* rangesExited, bool first ) const
{
  if (contains(pos)) {
    if (!first && rangesEntered)
      rangesEntered->push(const_cast<SmartRange*>(this));

    foreach (SmartRange* r, m_childRanges) {
      int result = r->positionRelativeToCursor(pos);
      if (result == 0)
        return r->deepestRangeContainingInternal(pos, rangesEntered, rangesExited);
      else if (result == 1)
        break;
    }

    return const_cast<SmartRange*>(this);

  } else {
    if (rangesExited)
      rangesExited->push(const_cast<SmartRange*>(this));

    if (!parentRange())
      return 0L;

    // first is true, because the parentRange won't be "entered" on first descent
    return parentRange()->deepestRangeContainingInternal(pos, rangesEntered, rangesExited, true);
  }
}

Document* SmartRange::document( ) const
{
  return smartStart().document();
}

void SmartRange::associateAction( KAction * action )
{
  m_associatedActions.append(action);

  bool enable = false;
  if (View* v = document()->activeView())
    if (contains(v->cursorPosition()))
      enable = true;

  action->setEnabled(enable);

  if (m_associatedActions.count() == 1)
    checkFeedback();
}

void SmartRange::dissociateAction( KAction * action )
{
  m_associatedActions.removeAll(action);
  if (!m_associatedActions.count())
    checkFeedback();
}

void SmartRange::clearAssociatedActions( )
{
  m_associatedActions.clear();
  checkFeedback();
}

SmartRange::InsertBehaviours SmartRange::insertBehaviour( ) const
{
  return (smartStart().moveOnInsert() ? DoNotExpand : ExpandLeft) | (smartEnd().moveOnInsert() ? ExpandRight : DoNotExpand);
}

void SmartRange::setInsertBehaviour(SmartRange::InsertBehaviours behaviour)
{
  static_cast<SmartCursor*>(m_start)->setMoveOnInsert(behaviour & ExpandLeft);
  static_cast<SmartCursor*>(m_end)->setMoveOnInsert(behaviour & ExpandRight);
}

void SmartRange::clearChildRanges()
{
  foreach (SmartRange* r, m_childRanges)
    r->removeText();
}

void SmartRange::deleteChildRanges()
{
  // FIXME: Probably more efficient to prevent them from unlinking themselves?
  qDeleteAll(m_childRanges);

  // i.e. this is probably already clear
  m_childRanges.clear();
}

void SmartRange::clearAndDeleteChildRanges( )
{
  // FIXME: Probably more efficient to prevent them from unlinking themselves?
  foreach (SmartRange* r, m_childRanges)
    r->removeText();

  qDeleteAll(m_childRanges);

  // i.e. this is probably already clear
  m_childRanges.clear();
}

void SmartRange::setParentRange( SmartRange * r )
{
  if (m_parentRange)
    m_parentRange->removeChildRange(this);

  m_parentRange = r;

  if (m_parentRange)
    m_parentRange->insertChildRange(this);
}

void SmartRange::setAttribute( Attribute * attribute, bool ownsAttribute )
{
  //if (m_attribute)
    //m_attribute->removeRange(this);

  if (m_ownsAttribute) delete m_attribute;

  m_attribute = attribute;
  m_ownsAttribute = ownsAttribute;

  //if (m_attribute)
    //m_attribute->addRange(this);
}

Attribute * SmartRange::attribute( ) const
{
  return m_attribute;
}

QStringList SmartRange::text( bool block ) const
{
  return document()->textLines(*this, block);
}

bool SmartRange::replaceText( const QStringList & text, bool block )
{
  return document()->replaceText(*this, text, block);
}

bool SmartRange::removeText( bool block )
{
  return document()->removeText(*this, block);
}

void SmartRange::rangeChanged( Cursor* c, const Range& from )
{
  Range::rangeChanged(c, from);

  // Decide whether the parent range has expanded or contracted, if there is one
  if (parentRange() && (start() < from.start() || end() > from.end()))
    parentRange()->expandToRange(*this);

  // Adjust sibling ranges if required
  if (parentRange()) {
    if (SmartRange* beforeRange = parentRange()->childBefore(this)) {
      if (beforeRange->end() > start())
        beforeRange->end() = start();
    }

    if (SmartRange* afterRange = parentRange()->childAfter(this)) {
      if (afterRange->start() < end())
        afterRange->start() = end();
    }
  }

  // Contract child ranges if required
  if (childRanges().count()) {
    SmartRange* r;
    QList<SmartRange*>::ConstIterator it;

    int i = 0;
    if (start() > from.start()) {
      // Start has contracted - adjust from the start of the child ranges
      for (; i < childRanges().count(); ++i) {
        r = childRanges().at(i);
        if (r->start() < start())
          r->confineToRange(*this);
        else
          break;
      }
    }

    if (end() < from.end()) {
      // End has contracted - adjust from the start of the child ranges, if they haven't already been adjusted above
      for (int j = childRanges().count() - 1; j >= i; --j) {
        r = childRanges().at(j);
        if (r->end() > end())
          r->confineToRange(*this);
        else
          break;
      }
    }
  }

  // SmartCursor and its subclasses take care of adjusting ranges if the tree structure is being used.
  if (hasNotifier() && notifier()->wantsDirectChanges()) {
    emit notifier()->positionChanged(this);
    emit notifier()->contentsChanged(this);

    if (start() == end())
      emit notifier()->eliminated(this);
  }

  if (watcher() && watcher()->wantsDirectChanges()) {
    watcher()->positionChanged(this);
    notifier()->contentsChanged(this);

    if (start() == end())
      notifier()->eliminated(this);
  }
}

bool SmartRange::isSmartRange( ) const
{
  return true;
}

SmartRange* SmartRange::toSmartRange( ) const
{
  return const_cast<SmartRange*>(this);
}

bool SmartRange::hasParent( SmartRange * parent ) const
{
  if (parentRange() == parent)
    return true;

  if (parentRange())
    return parentRange()->hasParent(parent);

  return false;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
