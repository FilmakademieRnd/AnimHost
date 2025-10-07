import os
import sys
import argparse


def replace_placeholders(template_str, collection_name, node_name):
    """Replace placeholders in a template string with the given collection name and node name."""

    template_str = template_str.replace('{NODE_NAME_UPPER}', node_name.upper()).replace('{NODE_NAME}', node_name)
    template_str = template_str.replace('{COLLECTION_NAME_UPPER}', collection_name.upper()).replace('{COLLECTION_NAME}', collection_name)
    return template_str

def write_file_from_template(template_path, output_path, collection_name, node_name):
    """Write a file from a template file, replacing placeholders with the given collection name and node name."""

    with open(template_path, 'r') as template_file:
        template_content = template_file.read()

    final_content = replace_placeholders(template_content, collection_name, node_name)

    with open(output_path, 'w') as output_file:
        output_file.write(final_content)


def create_plugin(collection_name: str, node_names: list[str]):
    """Create a new plugin with the given collection name and node names."""

    script_dir = os.path.dirname(os.path.abspath(__file__))  # Get the directory of the script
    plugin_dir = os.path.join(script_dir, f"{collection_name}Plugin")

    if not os.path.exists(plugin_dir):
        os.makedirs(plugin_dir)

    cmakelists_path = os.path.join(script_dir, "CMakeLists.txt")
    add_subdirectory_line = f"add_subdirectory({collection_name}Plugin)\n"

    with open(cmakelists_path, "r") as f:
        if add_subdirectory_line in f.read():
            print(f"'{collection_name}Plugin' already exists in CMakeLists.txt.")
        else:
            with open(cmakelists_path, "a") as f:
                f.write(add_subdirectory_line)

    json_content = "{}"
    with open(os.path.join(plugin_dir, f"{collection_name}Plugin.json"), "w") as f:
        f.write(json_content)

    templates_dir = os.path.join(script_dir, "templates")

    # Write collection files
    # Collection Headere
    write_file_from_template(os.path.join(templates_dir, 'NodeCollection.h.template'), 
                                     os.path.join(plugin_dir, f"{collection_name}Plugin.h"), collection_name, "")
    # Collection Source
    write_file_from_template(os.path.join(templates_dir, 'NodeCollection.cpp.template'), 
                                 os.path.join(plugin_dir, f"{collection_name}Plugin.cpp"), collection_name, "")
    # Collection Global Header
    write_file_from_template(os.path.join(templates_dir, 'NodeCollection_global.h.template'), 
                                 os.path.join(plugin_dir, f"{collection_name}Plugin_global.h"), collection_name, "")
    # Write node files
    node_file_string = ""
    for node_name in node_names:
        node_dir = os.path.join(plugin_dir, node_name)
        
        if not os.path.exists(node_dir):
            os.makedirs(node_dir)

        # Node Header
        write_file_from_template(os.path.join(templates_dir, 'Node.h.template'), 
                                 os.path.join(node_dir, f"{node_name}Node.h"),collection_name, node_name)
        # Node Source
        write_file_from_template(os.path.join(templates_dir, 'Node.cpp.template'), 
                                 os.path.join(node_dir, f"{node_name}Node.cpp"),collection_name, node_name)
        
        node_file_string += f"{node_name}/{node_name}Node.h {node_name}/{node_name}Node.cpp\n"
    
    # Write CMakeLists.txt
    write_file_from_template(os.path.join(templates_dir, 'NodeCollection.cmake.template'), 
                                 os.path.join(plugin_dir, "CMakeLists.txt"), collection_name, node_file_string)

    print(f"Plugin '{collection_name}' created successfully.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate files to build your own AnimHost plugin")
    # argument for the collection name
    parser.add_argument('collectionname', help="name of the node collection plugin")

    # argument for the node names (multiple)
    parser.add_argument('--nodes', nargs='+', help="names of the nodes")

    args = parser.parse_args()

    collection_name = args.collectionname
    node_names = args.nodes
    print(f"Collection Name: {collection_name}")
    if node_names:
        print(f"Node Names: {', '.join(node_names)}")
    else:
        print("No node names provided.")

    create_plugin(collection_name, node_names)
