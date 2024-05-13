# AnimHost Docs
## Setup

Follow these steps to generate the documentation on your local machine:

1. **Install Tools**: Install the necessary tools to generate the docs. Our setup uses Doxygen. Install it according to the instructions for your system: [Doxygen](https://www.doxygen.nl/download.html)

We use Sphinx to write and structure our documentation, with Breathe and Exhale as further extensions. We recommend using pip to install the Sphinx packages:
    ```
    pip install -U sphinx
    ```
And install the extensions we use:
    ```
    pip install breathe exhale
    ```

2. **Generate Doxygen XML**: In the `doc` folder, run Doxygen. The `Doxyfile` provides a valid configuration:
    ```
    doxygen
    ```

3. **Generate HTML Docs with Sphinx**: To generate the HTML documentation, simply run the following command in the `doc` folder:
    ```
    make html
    ```

4. **Access the Docs**: The HTML Documentation is placed inside the `doc/build/html` folder. Use `index.html` as an entry point.