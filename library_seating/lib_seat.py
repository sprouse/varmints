#!/usr/bin/env python

"""A simple python script template.

"""
import collections
import itertools
import os
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

def is_empty(p):
    return "Empty" in p.last

def spread_empty_locations(s):
    for j in range(6):
        for i in range(len(s)-1):
            if is_empty(s[i]) and is_empty(s[i+1]):
                print "Found:", i, i+1, s
                a = i+1
                b = i+5 % len(s)
                s[a], s[b] = s[b], s[a]


def assign_tables(groups, table_list):
    
    # Find the total capacity of all tables
    total_capacity = sum([x.size for x in table_list])

    # Find the total number of students
    total_number_students = sum([len(groups[g]['students']) for g in groups])
    
    estimated_extra_tables = (total_capacity - total_number_students) // 6
    
    # Figure out ahead of time how many empty tables there are.
    # Roughly distribute these empty tables over each of the groups
    group_cycle = itertools.cycle(list(sorted(groups.keys())))
    for i in range(estimated_extra_tables):
        g = group_cycle.next()
        print g
        for j in range(3):
            groups[g]['students'].insert(len(groups[g]['students'])//4*(j), Student('EmptyT', 'EmptyT', g, None, None))
    new_total_number_students = sum([len(groups[g]['students']) for g in groups])
    
    for k in sorted(groups):
        print k
        students = groups[k]['students']
        n = len(students)
        
        # Figure out which tables are assigned to each group
        # Allocate enough tables for the number of students in the group.
        groups[k]['tables'] = []
        capacity = 0
        while capacity < n:
            table = table_list.pop(0)
            groups[k]['tables'].append(table)
            capacity += table.size
        extra = capacity - n
        print('group ={}, cap={}, extra={}: {}'.format(k, capacity, extra, groups[k]['tables']))
        
        for i in range(extra):
            students.insert(random.randrange(len(students)+1), Student('EmptyP', 'EmptyP', k, None, None))
        
        spread_empty_locations(students)
        
        student_iter = iter(students)
        l = []
        for table in groups[k]['tables']:
            for seat in range(table.size):
                student = next(student_iter)
                assigned = Student(student.last, student.first, k, table, seat)
                l.append(assigned)
        print('X {}: {}'.format(k, l))
        groups[k]['assigned'] = l
    print('unused tables {}'.format(table_list))
    return groups


def main(arguments):

    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('infile', help="Input file", type=argparse.FileType('r'))
    #parser.add_argument('-o', '--outfile', help="Output file",
                    #default=sys.stdout, type=argparse.FileType('w'))

    args = parser.parse_args(arguments)
    
    print args

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
