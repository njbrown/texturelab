scriptdir=$(dirname "$BASH_SOURCE");

scssdir=$scriptdir/scss;
cssdir=$scriptdir/css;

lblue="$(tput setaf 12)"
yellow="$(tput setaf 3)"
lyellow="$(tput setaf 11)"
lred="$(tput setaf 9)"
purple="$(tput setaf 129)"
bold="$(tput bold)"
clear="$(tput rmul)$(tput rmso)$(tput sgr0)$(tput setaf 7)"

function log() {
    if [[ $1 == "inf" ]]; then
        echo -e $lblue  "Info: "          "$2" $clear
    elif [[ $1 == "wrn" ]]; then
        echo -e $yellow "Warn: " $lyellow "$2" $clear
    elif [[ $1 == "err" ]]; then
        echo -e $lred   "Err : " $white   "$2" $clear
    fi
}

function print_help() {
    echo "$bold${purple}Texturelab$clear SCSS Compiler Script";
    echo "";
    echo "This script compiles all .scss files into .css files and can continuously watch for changes. For more information read the README.md.";
    echo "";
    echo "You need to have Dart Sass installed to use this script.";
    echo "";
    echo "usage: $BASH_SOURCE [<options>]";
    echo "";
    echo "The following options are available:";
    echo "-c, --compile  Compile all scss files.";
    echo "-w, --watch    Watch scss files and recompile when they change.";
    echo "-h, -?, --help Print this usage information.";
}

function check_exit_code() {
    exit_code=$?
    if (( $exit_code == 127 )); then
        log err "You do not have the Dart Sass commandline tools installed or they are not available in the PATH";
        log inf "To install Dart Sass run:"
        log inf "   choco install sass          - windows"
        log inf "   brew install sass/sass/sass - mac/linux"
        log inf "   npm install -g sass         - js version"
    elif (( $exit_code == 0 )); then
        log inf "done";
    fi
}

if (( $# == 0 ));
then
    print_help;
else
    if [ $1 == "-h" ] || [ $1 == "-?" ] || [ $1 == "--help" ]; then
        print_help;
    elif [ $1 == "-c" ] || [ $1 == "--compile" ]; then
        log inf "compiling scss . . .";
        sass $scssdir:$cssdir --color;
        check_exit_code;
    elif [ $1 == "-w" ] || [ $1 == "--watch" ]; then
        log inf "watching scss . . ."
        sass $scssdir:$cssdir --color --watch;
    else
        log err "$1 is not an option"
    fi
fi