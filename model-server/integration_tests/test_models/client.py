#!/usr/bin/env python2.7
import argparse
import client_lib


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--server",
                        help="PredictionService in 'host:port' format",
                        required=True)
    parser.add_argument("--model", help="Model's name",
                        required=True)
    parser.add_argument("--version", help="Model's version",
                        type=int)
    parser.add_argument("--input_value", help="Input value for the model",
                        type=int, default=6)
    return parser.parse_args()


def main():
    args = parse_args()
    print(client_lib.make_request(args.server,
                                  args.input_value,
                                  args.model,
                                  args.version))


if __name__ == "__main__":
    main()
