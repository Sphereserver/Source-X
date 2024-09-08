#!/bin/bash

# Check if a parameter is provided
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 [--all|--changed|--last-commit] [--autocommit]"
    exit 1
fi

action=$1
project_root_dir="$(git rev-parse --show-toplevel)"

# Define the gersemi command; change the execution folder in order to make it read .gersemirc 
gersemi_cmd='env PWD="$project_root_dir" gersemi -i "{}"'
#gersemi_cmd='gersemi -i "{}" --list-expansion favour-inlining --no-warn-about-unknown-commands'

# Replace tabs with 4 spaces
sed_cmd='sed -i "s/\t/    /g" "{}"'

#looks like cmake-format is unmantained
#cmake-format CMakeLists.txt --separate-ctrl-name-with-space --separate-fn-name-with-space --dangle-parens --max-subgroups-hwrap=2 --max-pargs-hwrap=4 --max-rows-cmdline=120

# Function to format files
format_files() {
    echo "$1" | \
    xargs -P 4 -I {} sh -c '
        if [ -f "{}" ]; then
            echo "Formatting {}"
            '"$gersemi_cmd"'
            '"$sed_cmd"'
        else
            echo "{} does not exist?"
        fi
    '
    #if git diff --exit-code "{}" > /dev/null; then
    #    result="$result CHANGED"
    #else
    #    result="$result unchanged"
    #fi
}

# Case to determine the files based on the action
case $action in
    --all)
        # Find all *.cmake files and CMakeLists.txt
        files=$(find "$project_root_dir" \
          -type d -name 'CMakeFiles' -prune -o \
          -type d -name 'build' -prune -o \
          -type d -name '.*' -prune -o \
          -type f \( -name "*.cmake" -o -name "CMakeLists.txt" \) -print)
        ;;
    --changed)
        # Get modified and staged files (combined)
        unstaged_files=$(git diff --name-only)
        staged_files=$(git diff --cached --name-only)
        files=$( (echo "$unstaged_files" && echo "$staged_files") | sort | uniq | grep -E '\.cmake$|CMakeLists\.txt$')
        ;;
    --last-commit)
        # Get only the files modified by the last commit
        files=$( git diff --name-only HEAD~1 HEAD | grep -E '\.cmake$|CMakeLists\.txt$' )
        ;;
    *)
        echo "Invalid option."
        exit 1
        ;;
esac

# Call the formatting function
format_files "$files"

# Check if --autocommit flag is passed
if [[ "$2" == "--autocommit" ]]; then
  echo "$files" | xargs git add
  #git commit --amend --no-edit
  git commit --no-edit -m "Auto commit: re-formatted cmake files."
  echo "Previous commit amended with the staged files."
fi
