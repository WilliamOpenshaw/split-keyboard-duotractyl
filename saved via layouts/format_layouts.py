import json
import os
import re

# Define the DuoTractyl row structure (keys per row segment)
# Pattern: 7-7-7-6-3-2-3 per half
ROW_SIZES = [7, 7, 7, 6, 3, 2, 3]

def format_layer(layer_list):
    """Formats a single layer list into structured rows without comments."""
    output = ["[\n"]
    idx = 0
    
    # Process both halves
    for side_index in range(2): # 0 for Left, 1 for Right
        for size in ROW_SIZES:
            row_keys = layer_list[idx : idx + size]
            # Wrap keys in quotes and join with commas
            formatted_row = ", ".join(f'"{k}"' for k in row_keys)
            
            # Logic for commas: Add a comma if it's not the last key in the entire layer
            suffix = "," if idx + size < len(layer_list) else ""
            output.append(f"      {formatted_row}{suffix}\n")
            idx += size
            
        if side_index == 0:
            output.append("\n") # Blank line between halves for readability
            
    output.append("    ]")
    return "".join(output)

def process_files():
    for filename in os.listdir('.'):
        # Only process layout JSONs, avoid design files
        if filename.endswith('.json') and "design" not in filename.lower():
            try:
                with open(filename, 'r') as f:
                    content = f.read()

                # NEW: Remove all // style comments from the file content before parsing
                # This cleans up old comments like "// Left Half"
                content = re.sub(r'//.*', '', content)
                
                # Parse the cleaned content
                data = json.loads(content)
                
                if 'layers' in data and isinstance(data['layers'], list):
                    print(f"Cleaning and formatting {filename}...")
                    
                    layers_strings = []
                    for layer in data['layers']:
                        layers_strings.append(format_layer(layer))
                    
                    # Create placeholder
                    data['layers'] = "PLACEHOLDER"
                    final_json = json.dumps(data, indent=2)
                    
                    # Inject our custom-formatted block
                    layers_block = ",\n  \"layers\": [\n    " + ",\n    ".join(layers_strings) + "\n  ]"
                    final_content = re.sub(r',\s+"layers": "PLACEHOLDER"', layers_block, final_json)
                    
                    with open(filename, 'w') as f:
                        f.write(final_content)
            except Exception as e:
                print(f"Skipped {filename}: {e}")

if __name__ == "__main__":
    process_files()