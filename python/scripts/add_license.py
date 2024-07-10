import os

def add_license_to_file(file_path, license_text):
    """
    Add license text to the beginning of a file if not already present.
    """
    with open(file_path, 'r') as original_file:
        original_content = original_file.read()
    
    if license_text not in original_content:
        with open(file_path, 'w') as modified_file:
            modified_file.write(license_text + '\n' + original_content)
        print(f'License added to: {file_path}')
    else:
        print(f'License already present in: {file_path}')

def process_directory(root_dir, exclude_dirs, license_text):
    """
    Traverse the directory and add license to header files.
    """
    for subdir, dirs, files in os.walk(root_dir):
        # Skip the excluded directories
        dirs[:] = [d for d in dirs if os.path.join(subdir, d) not in exclude_dirs]
        
        for file in files:
            if file.endswith(('.h', '.hpp')):
                file_path = os.path.join(subdir, file)
                add_license_to_file(file_path, license_text)

if __name__ == '__main__':
    # Define your license text here
    license_text = """/*
 ***************************************************************************************

 *   Copyright (c) 2024 Filmakademie Baden-Wuerttemberg, Animationsinstitut R&D Labs
 *   https://research.animationsinstitut.de/animhost
 *   https://github.com/FilmakademieRnd/AnimHost
 *    
 *   AnimHost is a development by Filmakademie Baden-Wuerttemberg, Animationsinstitut
 *   R&D Labs in the scope of the EU funded project MAX-R (101070072).
 *    
 *   This program is distributed in the hope that it will be useful, but WITHOUT
 *   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *   FOR A PARTICULAR PURPOSE. See the MIT License for more details.
 *   You should have received a copy of the MIT License along with this program; 
 *   if not go to https://opensource.org/licenses/MIT

 ***************************************************************************************
 */

 """
    # Define the root directory of your project
    root_dir = 'C:\DEV\AnimHost\AnimHost'


    # Define directories to exclude
    exclude_dirs = [
        os.path.join(root_dir, 'core\QTNodes'),
        os.path.join(root_dir, 'animHost_Plugins\BasicOnnxPlugin\onnxruntime'),
    ]
    print(exclude_dirs)
    # Start processing the directory
    process_directory(root_dir, exclude_dirs, license_text)