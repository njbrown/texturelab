# How to use the compile.sh script

The comiple.sh script compiles all .scss files without a _ in the beginning into .css files.

These files will be compiled:

    scrollbar.scss --> scrollbar.css
         main.scss --> main.css

and these will be ignored:

        _lite.scss --!
        _dark.scss --!

## Windows
To run the script under Windows you need to have bash installed and added to the PATH.
If you have bash you can run the script by putting **bash** in front of the path to the .sh file you want to run.

    cd <Absolute path to /styles>
    bash ./compile.sh

or:

    bash <Absolute path to /styles>/compile.sh

## Linux and MacOS
To run the 

    cd <Absolute path to /styles>
    ./compile.sh

or:

    <Absolute path to /styles>/compile.sh

## Usage

    ./compile.sh [<options>]

The following options are available:

    -c, --compile  Compile all scss files.
    -w, --watch    Watch scss files and recompile when they change.
    -h, -?, --help Print usage information.
