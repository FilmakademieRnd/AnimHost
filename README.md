# AnimHub

## Setup

Follow these steps to set up the project on your local machine:

1. **Clone the repository** You can clone the repository and it's submodules by running the following command in your terminal:
    ```
    git clone --recurse-submodules https://github.com/FilmakademieRnd/AnimHost.git
    ```
2. **Navigate to the project directory** - Change your current directory to the project directory:
    ```
    cd AnimHost
    ```
3. **Create a build directory** - Create a new directory named `build` in the project directory:
    ```
    mkdir build
    ```
4. **Navigate to the build directory** - Change your current directory to the `build` directory:
    ```
    cd build
    ```
5. **Run CMake** - Run CMake to generate the build files, specifying Visual Studio as the generator. This project uses vcpkg for third-party dependencies, so this step might take some time. Replace `../AnimHost` with the path to the source code if it's not in the parent directory:
    ```
    cmake -G "Visual Studio 17 2022" ../AnimHost
    ```
6. **Build the project** - Finally, you can build the project using the generated build files. You can specify the configuration (Debug, Release, etc.) with the `--config` option:
    ```
    cmake --build . --config Release
    ```