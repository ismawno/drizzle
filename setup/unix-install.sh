#!/bin/bash
python_candidates=("python" "python3" "py")

working_python=""

for cmd in "${python_candidates[@]}"; do
    echo "Checking python executable: '$cmd'"
    if command -v "$cmd" > /dev/null 2>&1 && "$cmd" --version > /dev/null 2>&1; then
        working_python="$cmd"
        echo "Found working Python command: $working_python"
        break
    fi
done

if [ -z "$working_python" ]; then
    echo "Python is required to run the setup."
    exit 1
fi

SOURCE=${BASH_SOURCE[0]}
while [ -L "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
  DIR=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )
  SOURCE=$(readlink "$SOURCE")
  [[ $SOURCE != /* ]] && SOURCE=$DIR/$SOURCE # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done
DIR=$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )
ROOT="$DIR/.."

if [[ "$(uname)" == "Darwin" ]]; then
    $working_python "$ROOT/setup/setup.py" -s --xcode-command-line-tools --vulkan-sdk --cmake --build-script "$ROOT/setup/build.py" --build-type Dist -v --build-command make -j 4
else
    $working_python "$ROOT/setup/setup.py" -s --linux-devtools --vulkan-sdk --cmake --build-script "$ROOT/setup/build.py" --build-type Dist -v --build-command make -j 4
fi
