#!/usr/bin/env python

import os
import ast
import sys
import json
import math
import time
import pandas as pd
import shutil
import subprocess
import argparse


def PreProcess(filename, ofilename):

    class INPUT:
        size = 5

        wkey = 0
        tkey = 1
        msec = 2
        bytes = 3
        flops = 4

    class STATS:
        size = 5

        msec_min = 0
        msec_med = 1
        msec_max = 2
        bytes = 3
        flops = 4

    MEGA = 1000*1000
    GIGA = 1000*1000*1000

    data = dict()

    metadata = {}

    # read and process all the lines in the file
    for l in open(filename).readlines():
        m = l.split()

        is_metadata = False
        if len(m) > 0 and m[0].isupper():
            metadata[l[:-1]] = 1
            is_metadata = True

        # identify the number of threads
        if len(m) == 2 and (m[0] == "OPENMP_THREADS" or m[0] == "HVX_THREADS") :
            threads = int(m[1])

        # process a line of entry
        if not is_metadata and len(m) == INPUT.size:
            try:
                wkey = int(m[INPUT.wkey])
                if not data.has_key(wkey):
                    data[wkey] = dict()

                tkey = int(m[INPUT.tkey])
                if not data[wkey].has_key(tkey):
                    data[wkey][tkey] = STATS.size*[0]
                    first = True
                else:
                    first = False

                entry = data[wkey][tkey]

                msec  = float(m[INPUT.msec ])
                bytes = int  (m[INPUT.bytes])
                flops = int  (m[INPUT.flops])

                if first:
                    entry[STATS.msec_min] = msec
                    entry[STATS.msec_med] = [msec]
                    entry[STATS.msec_max] = msec
                else:
                    if msec < entry[STATS.msec_min]:
                        entry[STATS.msec_min] = msec
                    entry[STATS.msec_med].append(msec)
                    if msec > entry[STATS.msec_max]:
                        entry[STATS.msec_max] = msec

                entry[STATS.bytes] = bytes
                entry[STATS.flops] = flops

            except ValueError:
                pass

    # now that we have collected the stats data, write it out to file
    with open(ofilename, 'w') as ofile:
        for wkey in sorted(data.iterkeys()):
            tdict = data[wkey]
            for tkey in sorted(tdict.iterkeys()):
                stats = tdict[tkey]

                msec_min = stats[STATS.msec_min]

                msec_med = sorted(stats[STATS.msec_med])
                msec_med = msec_med[len(msec_med)/2]

                msec_max = stats[STATS.msec_max]

                gbytes = float(stats[STATS.bytes])/GIGA
                gflops = float(stats[STATS.flops])/GIGA

                if msec_min != 0.0 and msec_med != 0.0 and msec_max != 0.0:
                    GB_sec_min = gbytes/(msec_max/MEGA)
                    GB_sec_med = gbytes/(msec_med/MEGA)
                    GB_sec_max = gbytes/(msec_min/MEGA)

                    GFLOP_sec_min = gflops/(msec_max/MEGA)
                    GFLOP_sec_med = gflops/(msec_med/MEGA)
                    GFLOP_sec_max = gflops/(msec_min/MEGA)

                    ostr = "{} {} {} {} {} {} {} {} {} {} {}".format(
                        wkey,
                        tkey,
                        msec_min,
                        msec_med,
                        msec_max,
                        GB_sec_min,
                        GB_sec_med,
                        GB_sec_max,
                        GFLOP_sec_min,
                        GFLOP_sec_med,
                        GFLOP_sec_max)

                    ofile.write(ostr + "\n")

            ostr = "" + "\n"
            ofile.write(ostr)

        ostr = "META_DATA\n"
        ofile.write(ostr)
        for k,m in metadata.items():
            if k != "META_DATA":
                ofile.write(k + "\n")

    return ofilename

def FindMaximum(filename, ofilename):

    field0 = ""
    lastLine = ""

    found_metadata = False

    with open(ofilename, 'w') as ofile:
        for l in open(filename).readlines():
            m = l.split()

            if len(m) > 0 and m[0] == "META_DATA":
                found_metadata = True

                str = lastLine + "\n"
                ofile.write(str)

            if found_metadata:
                str = l
                ofile.write(str)
            else:
                if len(m) == 11 and m[0][0] != "#":
                    if m[0] != field0:
                        if lastLine != "":
                            str = lastLine
                            ofile.write(str)
                        field0 = m[0]
                    lastLine = l

    return ofilename

def Summarize(filename, ofilename):

    def smooth(x,y):
        xs = x[:]
        ys = y[:]

        d = 0

        for i in xrange(0,len(ys)):
            num = min(len(ys),i+d+1) - max(0,i-d)
            total = sum(ys[max(0,i-d):min(len(ys),i+d+1)])
            ys[i] = total/float(num)

        return xs,ys

    lines = open(filename).readlines()

    for i in xrange(0,len(lines)):
        if lines[i] == "META_DATA\n":
            break

    meta_lines = lines[i:]
    lines = lines[:i-1]

    x      = [float(line.split()[0]) for line in lines]
    band   = [float(line.split()[6]) for line in lines]
    gflops = [float(line.split()[9]) for line in lines]

    weight = 0.0

    with open(ofilename, 'w') as ofile:
        for i in xrange(0,len(x)-1):
            x1 = math.log(x[i])
            y1 = band[i]

            x2 = math.log(x[i+1])
            y2 = band[i+1]

            weight += (y1+y2)/2.0 * (x2-x1)

        maxband = max(band)
        start = band.index(maxband)

        x = x[start:]
        band = band[start:]

        minband = min(band)
        maxband = max(band)

        maxgflops = max(gflops)

        fraction = 1.05

        samples = 10000
        dband = maxband/float(samples - 1)

        counts = samples*[0]
        totals = samples*[0.0]

        x,band = smooth(x,band)

        for i in xrange(0,samples):
            cband = i*dband

            for v in band:
                if v >= cband/fraction and v <= cband*fraction:
                    totals[i] += v
                    counts[i] += 1

        ostr = "  %7.2f GFLOPs\n\n" % maxgflops
        ofile.write(ostr)

        band_list = [[1000*maxband,1000]]

        maxc = -1
        maxi = -1
        for i in xrange(samples-3,1,-1):
            if counts[i] > 6:
                if counts[i] > maxc:
                    maxc = counts[i]
                    maxi = i
            else:
                threshold = 1.25
                if maxc > 1:
                    value = float(totals[maxi])/max(1,counts[maxi])
                    if threshold*value < float(band_list[-1][0])/band_list[-1][1]:
                        band_list.append([totals[maxi],counts[maxi]])
                    else:
                        band_list[-1][0] += totals[maxi]
                        band_list[-1][1] += counts[maxi]
                maxc = -1
                maxi = -1

        ostr = "  %7.2f Weight\n\n" % weight
        ofile.write(ostr)

        band_name_list = ["DRAM"]
        cache_num = len(band_list)-1

        for cache in xrange(1,cache_num+1):
            band_name_list = ["L%d" % (cache_num+1 - cache)] + band_name_list

        for (band,band_name) in zip(band_list,band_name_list):
            ostr = "  %7.2f %s\n" % (float(band[0])/band[1],band_name)
            ofile.write(ostr)

        ostr = "\n"
        ofile.write(ostr)
        for m in meta_lines:
            ostr = m
            ofile.write(ostr)

    return ofilename

def Roofline(filelist, ofilename, json_filename):
    """Takes a list of files and processes them all together to determine the roofline."""

    max_gflops_value = 0.0
    max_gflops_name = ""

    max_weight = 0.0

    save_band = False
    max_band = []

    save_band_meta = False
    band_metadata = []

    save_flop_meta = False
    flop_metadata = []

    found_meta = False

    with open(ofilename, 'w') as ofile:
        for file in filelist:
            for line in open(file).readlines():
                parts = line.split()

                if save_band_meta or save_flop_meta:
                    if line[:-1] == "META_DATA":
                        found_meta = True

                        if save_band_meta:
                            band_metadata = []

                        if save_flop_meta:
                            flop_metadata = []

                    if found_meta:
                        if len(parts) == 2 and parts[1] == "GFLOPs":
                            save_band = False

                            save_band_meta = False
                            save_flop_meta = False

                            found_meta = False
                        else:
                            if save_band_meta:
                                band_metadata.append(line[:-1])

                            if save_flop_meta:
                                flop_metadata.append(line[:-1])

                if not found_meta:
                    if len(parts) == 2:
                        if parts[1] == "GFLOPs":
                            gflops_value = float(parts[0])
                            gflops_name = parts[1]

                            if gflops_value > max_gflops_value:
                                max_gflops_value = gflops_value
                                max_gflops_name = gflops_name

                                save_flop_meta = True
                                found_meta = False
                        elif parts[1] == "Weight":
                            weight = float(parts[0])

                            if weight > max_weight:
                                max_weight = weight

                                save_band = True
                                max_band = []

                                save_band_meta = True
                                found_meta = False
                        elif save_band:
                            max_band.append(line[:-1])

        ostr = "  %7.2f %s EMP\n" % (max_gflops_value, max_gflops_name)
        ofile.write(ostr)

        for m in flop_metadata:
            ostr = m + "\n"
            ofile.write(ostr)

        ostr = "\n"
        ofile.write(ostr)

        for b in max_band:
            ostr = "%s EMP\n" % b
            ofile.write(ostr)

        for m in band_metadata:
            ostr = m + "\n"
            ofile.write(ostr)

    with open(ofilename, 'r') as ofile:
        lines = [x.strip() for x in ofile.readlines()]

        for i in xrange(0, len(lines)):
            if len(lines[i]) == 0:
                break

        gflop_lines = lines[:i]
        gbyte_lines = lines[i + 1:-1]

        database = build_database(gflop_lines, gbyte_lines)
        try:
            database_file = open(json_filename, "w")
        except IOError:
            sys.stderr.write("Unable to open database file, %s\n" % json_filename)
            return 1

        json.dump(database, database_file, indent=3)

        database_file.close()

    return ofilename, json_filename

def merge_metadata(in_sub_meta1, in_sub_meta2):
    out_meta = {}
    out_sub_meta1 = {}
    out_sub_meta2 = {}

    in_sub_k1 = set(in_sub_meta1.keys())
    in_sub_k2 = set(in_sub_meta2.keys())

    out_k = in_sub_k1 & in_sub_k2

    out_sub_k1 = in_sub_k1 - in_sub_k2
    out_sub_k2 = in_sub_k2 - in_sub_k1

    for k in out_k:
        if in_sub_meta1[k] == in_sub_meta2[k]:
            out_meta[k] = in_sub_meta1[k]
        else:
            out_sub_k1.add(k)
            out_sub_k2.add(k)

    for k in out_sub_k1:
        out_sub_meta1[k] = in_sub_meta1[k]

    for k in out_sub_k2:
        out_sub_meta2[k] = in_sub_meta2[k]

    return(out_meta,out_sub_meta1,out_sub_meta2)

def build_database(gflop, gbyte):
    gflop0 = gflop[0].split()

    emp_gflops_data = []
    emp_gflops_data.append([gflop0[1], float(gflop0[0])])

    emp_gflops_metadata = {}
    for metadata in gflop[1:]:
        parts = metadata.partition(" ")
        key = parts[0].strip()
        if key != "META_DATA":
            try:
                new_value = ast.literal_eval(parts[2].strip())
            except (SyntaxError,ValueError):
                new_value = parts[2].strip()

            if key in emp_gflops_metadata:
                value = emp_gflops_metadata[key]

                if isinstance(value,list):
                    value.append(new_value)
                else:
                    value = [value,new_value]

                emp_gflops_metadata[key] = value
            else:
                emp_gflops_metadata[parts[0].strip()] = new_value

    emp_gflops_metadata["TIMESTAMP_DB"] = time.time()

    emp_gflops = {}
    emp_gflops['data'] = emp_gflops_data

    emp_gbytes_metadata = {}
    emp_gbytes_data = []

    for i in xrange(0,len(gbyte)):
        if gbyte[i] == "META_DATA":
            break
        else:
            gbyte_split = gbyte[i].split()
            emp_gbytes_data.append([gbyte_split[1],float(gbyte_split[0])])

    for j in xrange(i+1,len(gbyte)):
        metadata = gbyte[j]

        parts = metadata.partition(" ")
        key = parts[0].strip()
        if key != "META_DATA":
            try:
                new_value = ast.literal_eval(parts[2].strip())
            except (SyntaxError,ValueError):
                new_value = parts[2].strip()

            if key in emp_gbytes_metadata:
                value = emp_gbytes_metadata[key]

                if isinstance(value,list):
                    value.append(new_value)
                else:
                    value = [value,new_value]

                emp_gbytes_metadata[key] = value
            else:
                emp_gbytes_metadata[parts[0].strip()] = new_value

    emp_gbytes_metadata["TIMESTAMP_DB"] = time.time()

    emp_gbytes = {}
    emp_gbytes['data'] = emp_gbytes_data

    (emp_metadata,emp_gflops_metadata,emp_gbytes_metadata) = merge_metadata(emp_gflops_metadata,emp_gbytes_metadata)

    emp_gflops['metadata'] = emp_gflops_metadata
    emp_gbytes['metadata'] = emp_gbytes_metadata

    empirical = {}
    empirical['metadata'] = emp_metadata
    empirical['gflops']    = emp_gflops
    empirical['gbytes']    = emp_gbytes

    spec_gflops_data = []
    spec_gflops = {}
    spec_gflops['data'] = spec_gflops_data

    spec_gbytes_data = []
    spec_gbytes = {}
    spec_gbytes['data'] = spec_gbytes_data

    spec = {}
    spec['gflops'] = spec_gflops
    spec['gbytes'] = spec_gbytes

    result = {}
    result['empirical'] = empirical
    result['spec']      = spec

    return result

def PlotFlops(ifilename, results_dir, gnu_dir):

    assert(os.path.exists(os.path.join(os.getcwd(), results_dir)))
    assert(os.path.exists(os.path.join(os.getcwd(), gnu_dir)))
    assert(os.path.isdir(results_dir))
    assert(os.path.isdir(gnu_dir))

    # load all the lines, and split them into columns
    lines = [x.strip().split() for x in open(ifilename).readlines()]
    # an empty line follows METADATA stuff, so we want to stop before that
    for i in xrange(0, len(lines)):
        if len(lines[i]) == 0:
            break

    db_lines = lines[:i] #ignore the META_DATA lines
    df = pd.DataFrame(db_lines, columns=['wkey', 'tkey',
                                         'msec_min', 'msec_med', 'msec_max',
                                         'GB_sec_min', 'GB_sec_med', 'GB_sec_max',
                                         'GFLOP_sec_min', 'GFLOP_sec_med', 'GFLOP_sec_max'])

    df = df.apply(pd.to_numeric)

    xmin = df['wkey'].min()
    xmax = df['wkey'].max()
    ymin = df['GFLOP_sec_med'].min()
    ymax = df['GFLOP_sec_med'].max()

    basename = "flops-vs-wss"
    loadname = "%s/flops_%s.gnu" % (results_dir, os.path.basename(ifilename))
    #loadname = "%s/%s.gnu" % (results_dir, basename)

    title = "GFLOP/s vs. Working Set Size"

    command = "sed "
    command += "-e 's#ERT_TITLE#%s#g' " % title
    command += "-e 's#ERT_XRANGE_MIN#%le#g' " % xmin
    command += "-e 's#ERT_XRANGE_MAX#%le#g' " % xmax
    command += "-e 's#ERT_YRANGE_MIN#%le#g' " % ymin
    command += "-e 's#ERT_YRANGE_MAX#\*#g' "
    command += "-e 's#ERT_GRAPH#%s#g' " % loadname
    command += "-e 's#ERT_MAX_DATA#%s#g' " % ifilename

    command += "< %s/%s.gnu.template > %s" % (gnu_dir, basename, loadname)
    p = subprocess.Popen(command, shell=True)
    os.waitpid(p.pid, 0)

    command = "gnuplot %s" % loadname
    p = subprocess.Popen(command, shell=True)
    os.waitpid(p.pid, 0)

    return loadname

def PlotBandwidth(ifilename, results_dir, gnu_dir):

    assert(os.path.exists(os.path.join(os.getcwd(), results_dir)))
    assert(os.path.exists(os.path.join(os.getcwd(), gnu_dir)))
    assert(os.path.isdir(results_dir))
    assert(os.path.isdir(gnu_dir))

    # load all the lines, and split them into columns
    lines = [x.strip().split() for x in open(ifilename).readlines()]
    # an empty line follows METADATA stuff, so we want to stop before that
    for i in xrange(0, len(lines)):
        if len(lines[i]) == 0:
            break

    db_lines = lines[:i] #ignore the META_DATA lines
    df = pd.DataFrame(db_lines, columns=['wkey', 'tkey',
                                         'msec_min', 'msec_med', 'msec_max',
                                         'GB_sec_min', 'GB_sec_med', 'GB_sec_max',
                                         'GFLOP_sec_min', 'GFLOP_sec_med', 'GFLOP_sec_max'])

    df = df.apply(pd.to_numeric)

    xmin = df['wkey'].min()
    xmax = df['wkey'].max()
    ymin = df['GB_sec_med'].min()
    ymax = df['GB_sec_med'].max()

    basename = "band-vs-wss_max"
    loadname = "%s/bw_%s.gnu" % (results_dir, os.path.basename(ifilename))
    #loadname = "%s/%s.gnu" % (results_dir, basename)

    title = "GB/s vs. Working Set Size"

    command = "sed "
    command += "-e 's#ERT_TITLE#%s#g' " % title
    command += "-e 's#ERT_XRANGE_MIN#%le#g' " % xmin
    command += "-e 's#ERT_XRANGE_MAX#%le#g' " % xmax
    command += "-e 's#ERT_YRANGE_MIN#%le#g' " % ymin
    command += "-e 's#ERT_YRANGE_MAX#\*#g' "
    command += "-e 's#ERT_GRAPH#%s#g' " % loadname
    command += "-e 's#ERT_MAX_DATA#%s#g' " % ifilename

    command += "< %s/%s.gnu.template > %s" % (gnu_dir, basename, loadname)
    p = subprocess.Popen(command, shell=True)
    os.waitpid(p.pid, 0)

    command = "gnuplot %s" % loadname
    p = subprocess.Popen(command, shell=True)
    os.waitpid(p.pid, 0)

    return loadname

def PlotRoofline(filename, results_dir, gnu_dir):

    assert(os.path.exists(os.path.join(os.getcwd(), results_dir)))
    assert(os.path.exists(os.path.join(os.getcwd(), gnu_dir)))
    assert(os.path.isdir(results_dir))
    assert(os.path.isdir(gnu_dir))

    lines = [x.strip() for x in open(filename).readlines()]
    for i in xrange(0, len(lines)):
        if len(lines[i]) == 0:
            break

    gflop_lines = lines[:i]
    gbyte_lines = lines[i + 1:-1]

    line = gflop_lines[0].split()
    gflops_emp = [float(line[0]), line[1]]

    for i in xrange(0, len(gbyte_lines)):
        if gbyte_lines[i] == "META_DATA":
            break

    num_mem = i
    gbytes_emp = num_mem * [0]

    for i in xrange(0, num_mem):
        line = gbyte_lines[i].split()
        gbytes_emp[i] = [float(line[0]), line[1]]

    x = num_mem * [0.0]
    for i in xrange(0, len(gbytes_emp)):
        x[i] = gflops_emp[0] / gbytes_emp[i][0]

    basename = "roofline"
    loadname = "%s/%s.gnu" % (results_dir, basename)

    xmin = 0.01
    xmax = 100.00

    ymin = 10 ** int(math.floor(math.log10(gbytes_emp[0][0] * xmin)))

    title = "Empirical Roofline Graph"

    command = "sed "
    command += "-e 's#ERT_TITLE#%s#g' " % title
    command += "-e 's#ERT_XRANGE_MIN#%le#g' " % xmin
    command += "-e 's#ERT_XRANGE_MAX#%le#g' " % xmax
    command += "-e 's#ERT_YRANGE_MIN#%le#g' " % ymin
    command += "-e 's#ERT_YRANGE_MAX#\*#g' "
    command += "-e 's#ERT_GRAPH#%s/%s#g' " % (results_dir, basename)

    command += "< %s/%s.gnu.template > %s" % (gnu_dir, basename, loadname)
    os.system(command)

    try:
        plotfile = open(loadname, "a")
    except IOError:
        sys.stderr.write("Unable to open '%s'...\n" % loadname)
        return 1

    xgflops = 2.0
    label = '%.1f %s/sec (Maximum)' % (gflops_emp[0], gflops_emp[1])
    plotfile.write(
        "set label '%s' at %.7le,%.7le left textcolor rgb '#000080'\n" % (label, xgflops, 1.2 * gflops_emp[0]))

    xleft = xmin
    xright = x[0]

    xmid = math.sqrt(xleft * xright)
    ymid = gbytes_emp[0][0] * xmid

    y0gbytes = ymid
    x0gbytes = y0gbytes / gbytes_emp[0][0]

    C = x0gbytes * y0gbytes

    alpha = 1.065

    label_over = True
    for i in xrange(0, len(gbytes_emp)):
        if i > 0:
            if label_over and gbytes_emp[i - 1][0] / gbytes_emp[i][0] < 1.5:
                label_over = False

            if not label_over and gbytes_emp[i - 1][0] / gbytes_emp[i][0] > 3.0:
                label_over = True

        if label_over:
            ygbytes = math.sqrt(C * gbytes_emp[i][0]) / math.pow(alpha, len(gbytes_emp[i][1]))
            xgbytes = ygbytes / gbytes_emp[i][0]

            ygbytes *= 1.1
            xgbytes /= 1.1
        else:
            ygbytes = math.sqrt(C * gbytes_emp[i][0]) / math.pow(alpha, len(gbytes_emp[i][1]))
            xgbytes = ygbytes / gbytes_emp[i][0]

            ygbytes /= 1.1
            xgbytes *= 1.1

        label = "%s - %.1lf GB/s" % (gbytes_emp[i][1], gbytes_emp[i][0])

        plotfile.write(
            "set label '%s' at %.7le,%.7le left rotate by 45 textcolor rgb '#800000'\n" % (label, xgbytes, ygbytes))

    plotfile.write("plot \\\n")

    for i in xrange(0, len(gbytes_emp)):
        plotfile.write("     (x <= %.7le ? %.7le * x : 1/0) lc 1 lw 2,\\\n" % (x[i], gbytes_emp[i][0]))

    plotfile.write("     (x >= %.7le ? %.7le : 1/0) lc 3 lw 2\n" % (x[0], gflops_emp[0]))

    plotfile.close()

    return loadname

def GetFilenames(path, ext):
    """Get a list of all filenames in this directory."""
    files = []
    for file in os.listdir(path):
        if file.endswith(ext):
            files.append(os.path.join(path, file))

    return files

# todo: update all functions to write output to the results directory
# todo: add code to generate all the other gnuplots
def Main(idir, odir, gdir):
    if os.path.exists(odir):
        print("Deleting files in old results directory/: ".format(odir))
        shutil.rmtree(odir)

    print("Creating results output directory/: ".format(odir))
    os.mkdir(odir)

    filelist = GetFilenames(idir, '.gables')
    print("Processing file list: ".format(filelist))

    pre_ofilelist = []
    max_ofilelist = []
    sum_ofilelist = []
    for filename in filelist:
        print("Processing {}".format(filename))

        print("\t PreProcessing...")
        pre_ofilename = os.path.join(odir, os.path.basename(filename)) + ".pre"
        PreProcess(filename, pre_ofilename)
        pre_ofilelist.append(pre_ofilename)

        print("\t FindingMax...")
        max_ofilename = os.path.join(odir, os.path.basename(pre_ofilename)) + ".max"
        FindMaximum(pre_ofilename, max_ofilename)
        max_ofilelist.append(max_ofilename)

        print("\t Summarizing...")
        sum_ofilename = os.path.join(odir, os.path.basename(max_ofilename)) + ".sum"
        Summarize(max_ofilename, sum_ofilename)
        sum_ofilelist.append(sum_ofilename)

        print("\t Plotting...")
        band_plot_filename = PlotBandwidth(max_ofilename, odir, gdir)
        flop_plot_filename = PlotFlops(max_ofilename, odir, gdir)


    print("Processing roofline results...")
    roofline_raw_output = os.path.join(odir, "roofline.txt")
    roofline_json_output = os.path.join(odir, "roofline.json")
    Roofline(sum_ofilelist, roofline_raw_output, roofline_json_output)
    print("\t Results in " + roofline_raw_output)
    print("\t Results in " + roofline_json_output)

    print("Generating plots...")
    gnuplot_filename = PlotRoofline(roofline_raw_output, odir, gdir)
    print("\t Roofline plot is in " + gnuplot_filename)
    os.system('gnuplot %s' % gnuplot_filename)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Process some integers.')
    parser.add_argument('-d', '--directory', type=str, required=True, help='directory containing the files.')
    parser.add_argument('-o', '--output', default='output', type=str, help='directory containing the files.')
    parser.add_argument('-g', '--gnu', default='gnuplots', type=str, help='directory containing the files.')

    args = parser.parse_args()

    Main(args.directory, args.output, args.gnu)
