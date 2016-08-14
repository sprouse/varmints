#!/usr/bin/env python

"""A simple python script template.

"""

import os
import sys
import argparse


def main(arguments):

    	parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    	parser.add_argument('infile', help="Input file", type=argparse.FileType('r'))
    	#parser.add_argument('-o', '--outfile', help="Output file",
                        #default=sys.stdout, type=argparse.FileType('w'))

    	args = parser.parse_args(arguments)
	
    	print args

	table_sizes = [(4, 17), (5,8), (6,3)]

	total = 0
	for s, n in table_sizes:
		total += s*n
		print s, n, s*n
	print total	
	groups = {}
	for line in args.infile:
		student = line.split(',')
		groups.setdefault(student[2].strip(), []).append((student[0], student[1]))

	for g in groups:
		print g,":", len(groups[g])

if __name__ == '__main__':
    	sys.exit(main(sys.argv[1:]))
