# copyright 2015 by mike lodato (zvxryb@gmail.com)
# this work is subject to the terms of the MIT license

import argparse
import scan.config as config

def main():
	parser = argparse.ArgumentParser()
	subparsers = parser.add_subparsers(dest = 'command', title = 'commands')
	
	config_parser = subparsers.add_parser('config',
		help = 'run interactive config')
	config_parser.add_argument('path')
	
	args = parser.parse_args()
	if args.command == 'config':
		with open(args.path, 'wb') as f:
			config.config(f)

