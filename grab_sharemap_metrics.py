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

def main(schema, url):
    # parse sharemap schema for metrics
    y = yaml.safe_load(open(schema, 'r'))
    metrics_sharemap = sharemap_lib.Sharemap(y['metrics'])

    # create socket based on the url
    o = urlparse(url)
    if o.scheme == 'udp':
        s = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
        s.bind((o.hostname, o.port))
        buff = s.recv(2048)
    else:
        context = zmq.Context()
        s = context.socket(zmq.PULL)
        s.bind(url)
        buff = s.recv()

    # print metrics
    metrics = metrics_sharemap.unpack(buff)
    print(json.dumps(metrics, indent=4))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Grab and print sharemap metrics"
    )
    parser.add_argument("--schema",
        type=pathlib.Path,
        help="Path to the YAML schema",
        default=os.path.join(THIS_DIR, 'schema.yaml')
    )
    parser.add_argument("--url",
        type=str,
        help="socket bind url"
    )
    args = parser.parse_args()
    main(args.schema, args.url)
