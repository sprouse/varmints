#!/usr/bin/env python

"""A simple python script template.

"""
import collections
import csv
import itertools
import six
import sys
import argparse
import numpy.random
import random

Table = collections.namedtuple('Table', 'number size')
Student = collections.namedtuple('Student', 'last first group table seat')

numpy.random.seed(seed=1)

def create_table_list(table_types):
    l = []
    for s, n in table_types:
        l += [s] * n
    numpy.random.shuffle(l)
    table_list = []
    for i, s in enumerate(l):
        table_list.append(Table(number=i, size=s))
    return table_list


def main(arguments):

    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('infile', help="Input file", type=argparse.FileType('r'))
    #parser.add_argument('-o', '--outfile', help="Output file",
                    #default=sys.stdout, type=argparse.FileType('w'))

    args = parser.parse_args(arguments)
    
    print args

    table_reader = csv.reader(args.infile)

    for row in table_reader:
        print row

    table_sizes = [(4, 17), (5, 8), (6, 3)]

    table_list = create_table_list(table_sizes)

    print(table_list)

    tg = {}
    for line in args.infile:
        student = line.split(',')
        tg.setdefault(student[2].strip(), []).append(
            Student(last=student[0],
                    first=student[1],
                    group=student[2].strip(),
                    table=None,
                    seat=None))
        
    groups = {}
    for k in tg:
        v = tg[k]
        groups[k] = {}
        groups[k]['students'] = v

    for g in groups:
        l = groups[g]['students']
        l = sorted(l, key=lambda tup: tup.last)
        groups[g]['students'] = l
        print g, ":", len(groups[g]['students']), groups[g]['students']
        
    group2 = assign_tables(groups, table_list)
    
    for g in sorted(groups):
        print('########### {} ############'.format(g))
        for s in groups[g]['assigned']:
            print(s)
        
    exit(0)

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
