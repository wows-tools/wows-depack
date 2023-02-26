#!/bin/sh

if ! [ -d docs/doxygen-awesome-css ]
then
    curl -L https://github.com/jothepro/doxygen-awesome-css/archive/refs/tags/v2.2.0.tar.gz --output doxycss.tar.gz
    tar -xf doxycss.tar.gz -C docs/
    mv docs/doxygen-awesome-css* docs/doxygen-awesome-css
    rm -f doxycss.tar.gz
else
    echo "docs/doxygen-awesome-css already exists."
    echo "please run:"
    echo "   rm -rf \"docs/doxygen-awesome-css\""
    echo "to redeploy it."
fi
