# AnimHost
AnimHost connects animation generators to Digital Content Creation applications, on-set tools like VPET or Game Engines in general. It explores enhanced XR production processes for animated films, utilising machine learning with a “fair use of data” and an “artists in the loop” approach.
![AnimHost](/doc/resources/AnimHost_Shematic_1k.png)

**AninHost web site:** https://research.animationsinstitut.de/animhost

## Build Instructions

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
## About
![](/doc/resources/FA_AI_Logo.png) &nbsp;&nbsp;&nbsp;&nbsp;
![](/doc/resources/logo_rnd.jpg) &nbsp;&nbsp;&nbsp;&nbsp;
![](/doc/resources/Max-R_Logo.png)

AnimHost is a development by [Filmakademie Baden-Wuerttemberg](https://filmakademie.de/), [Animationsinstitut R&D Labs](http://research.animationsinstitut.de/) in the scope of the EU funded project [MAX-R](https://max-r.eu/) (101070072).

## Funding
![Animationsinstitut R&D](/doc/resources/EN_FundedbytheEU_RGB_POS_rs.png)

This project has received funding from the European Union's Horizon Europe Research and Innovation Programme under Grant Agreement No 101070072 MAX-R.

## License
AnimHost is a open-sorce development by Filmakademie Baden-Wuerttemberg's Animationsinstitut.  
The framework is licensed under [MIT](LICENSE.txt). See [License info file](LICENSE_Info.txt) for more details.
