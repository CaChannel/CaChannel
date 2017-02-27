#!/bin/bash

$PYTHON setup.py install

case `uname` in
    Darwin )
        CAREPEATER=$RECIPE_DIR/caRepeater.Darwin ;;
    Linux )
        CAREPEATER=$RECIPE_DIR/caRepeater.Linux$ARCH ;;
    * )
        echo "Not Supported"
esac

[[ -f ${PREFIX}/bin/caRepeater ]] || cp $CAREPEATER ${PREFIX}/bin/caRepeater

# Add more build steps here, if they are necessary.

# See
# http://docs.continuum.io/conda/build.html
# for a list of environment variables that are set during the build process.
