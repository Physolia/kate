#!/bin/bash
# This scripts uses pandoc to update the README.md file from the documentation.

cd $(dirname "$(readlink -f "$BASH_SOURCE")")

sed -n '/^<sect1 id="kate-application-plugin-keyboardmacros">$/,/^<\/sect1>$/p' ../../doc/kate/plugins.docbook | \
    pandoc -f docbook -t markdown -s - -o - | \
    sed 's/{.\(menuchoice\|keycombo\)}//;s/+K/K/;s/CTRL/Ctrl+/;s/SHIFT/Shift+/;s/ALT/Alt+/;s/(\[/[`/;s/\])/`]/' | \
    cat > README.md

cd - >/dev/null
