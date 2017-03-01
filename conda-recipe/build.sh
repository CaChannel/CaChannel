#!/bin/bash

OUTPUT_PATH=$(dirname $(conda build --output conda-recipe))

case `uname` in
    Darwin )
        CAREPEATER=$RECIPE_DIR/caRepeater.Darwin
        $PYTHON setup.py install bdist_wheel
        cp dist/*.whl ${OUTPUT_PATH}
        ;;
    Linux )
        CAREPEATER=$RECIPE_DIR/caRepeater.Linux$ARCH
        $PYTHON setup.py install sdist bdist_egg
        cp dist/*.tar.gz ${OUTPUT_PATH}
        cp dist/*.egg ${OUTPUT_PATH}
        ;;
    * )
        echo "Not Supported"
esac

[[ -f ${PREFIX}/bin/caRepeater ]] || cp $CAREPEATER ${PREFIX}/bin/caRepeater

# Add more build steps here, if they are necessary.

# See
# http://docs.continuum.io/conda/build.html
# for a list of environment variables that are set during the build process.
