#!/usr/bin/env python3

import sys
import sharemap_lib
import pathlib
import yaml
import zmq
import time
import json
import os
import socket
import argparse
from urllib.parse import urlparse

THIS_DIR = os.path.abspath(os.path.dirname(__file__))

def main(schema, url, config):
    # parse sharemap schema for config
    y = yaml.safe_load(open(schema, 'r'))
    config_sharemap = sharemap_lib.Sharemap(y['config'])

    # create config buffer
    config_map = yaml.safe_load(open(config, 'r'))
    buff = config_sharemap.pack(config_map)
    print(buff)

    # create socket based on the url
    o = urlparse(url)
    if o.scheme == 'udp':
        s = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
        s.connect((o.hostname, o.port))
        s.send(buff)
    else:
        context = zmq.Context()
        s = context.socket(zmq.PUSH)
        s.connect(url)
        s.send(buff)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Send sharemap config from yaml source"
    )
    parser.add_argument("--schema",
        type=pathlib.Path,
        help="Path to the YAML schema",
        default=os.path.join(THIS_DIR, 'schema.yaml')
    )
    parser.add_argument("--url",
        type=str,
        help="sharemap server url"
    )
    parser.add_argument("--config",
        type=pathlib.Path,
        help="Path to the YAML config",
    )
    args = parser.parse_args()
    main(args.schema, args.url, args.config)
