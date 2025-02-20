#!/usr/bin/env python3

import pathlib
from sharemap_lib import Sharemap
import yaml
from jinja2 import Environment, FileSystemLoader

# Load YAML schema
def load_yaml_schema(schema_path):
    with open(schema_path, "r", encoding="utf-8") as file:
        return yaml.safe_load(file)

# Load Jinja templates
def load_template(template_path):
    env = Environment(loader=FileSystemLoader(str(template_path.parent)))
    return env.get_template(str(template_path.name))

# Generate C++ code from templates
def generate_code(schema, template):
    sharemaps = []

    for top_key, top_sharemap_config in schema.items():
        sharemap = Sharemap(schema.get(top_key, {}))
        sharemaps.append((top_key, sharemap))

    # Render class definitions
    class_definitions = template.render(sharemaps=sharemaps, Sharemap=Sharemap)

    return class_definitions


def main(schema_path, output_file_path, template_path):
    schema = load_yaml_schema(schema_path)
    template = load_template(template_path)
    class_definitions = generate_code(schema, template)

    # Ensure output directory exists
    output_file_path.parent.mkdir(parents=True, exist_ok=True)

    with open(output_file_path, "w", encoding="utf-8") as file:
        file.write(class_definitions)
    print(f"Generated code has been saved to {output_file_path}")


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(
        description="Generate C++ code from YAML schema using Jinja templates"
    )
    parser.add_argument("schema", type=pathlib.Path, help="Path to the YAML schema")
    parser.add_argument(
        "--output",
        type=pathlib.Path,
        default=pathlib.Path("./shaemap.hpp"),
        help="Output file name for generated C++ file",
    )
    parser.add_argument(
        "--template",
        type=pathlib.Path,
        default=pathlib.Path("./sharemap.cpp.jinja"),
        help="File name of Jinja template",
    )

    args = parser.parse_args()
    main(args.schema, args.output, args.template)
