import argparse
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('csource', help = 'C source file to protect. Ends with .c.')
parser.add_argument('connectivity_level', type = int, help = 'Integer specifying protection network connectivity level.')
parser.add_argument('sens_funcs', nargs='+', help = 'Names of sensitive functions. Strings separated with whitespace.')

args = parser.parse_args()
if args.csource.endswith('.c') is False:
  parser.print_usage()
  parser.print_help()
  parser.exit('Error: C source file should end with .c.')
print "Source file passed: " + args.csource
print "Connectivity level passed: " + str(args.connectivity_level)
print "Sensitive functions passed: " + str(args.sens_funcs)

pairs_file = open('data.dat', 'w')

for func in args.sens_funcs:
   klee_output = open('klee_output', 'w')
   p = subprocess.Popen(["python3", "/home/sip/klee/syminputC.py", func, args.csource], stdout = subprocess.PIPE)
   out, err = p.communicate()
   #print out
   lines = out.split('\n')
   numtests = 0
   #pairs = []
   next_pair = []
   name = ''
   for line in lines:
      if line.startswith('num objects:'):
         numtests = numtests + 1
      name_idx = line.find('name:')
      if name_idx != -1:
         splitted = line.split(':')
	 name = splitted[2].strip("' ")
      data_idx = line.find('data:')
      if data_idx != -1:
         splitted = line.split(':')
         if name.startswith(('macke', 'model')) is False:
            next_pair.append(splitted[2].strip())
         if name == 'macke_result':
	    next_pair.append(splitted[2].strip())
            #pairs.append(next_pair)
            line_to_write = func + ' ' + next_pair[0] + ' ' + next_pair[1] + '\n'
            print line_to_write
            pairs_file.write(line_to_write)
            next_pair = []
   print 'Number of pairs for function ' + func + ': ' + str(numtests)
   print 'Pairs have been written to data.dat file in current directory.'
  # print 'pairs for function ' + func + ':'
  # print pairs 
pairs_file.flush()

bcsource = args.csource.replace('.c', '.bc')
clang_call = subprocess.Popen(['clang-3.9', '-o3', '-emit-llvm', args.csource, '-c', '-o', bcsource], stdout = subprocess.PIPE)
out, err = clang_call.communicate();

opt_args = 'opt-3.9 -load pass/libResultCheckingPass.so -instr -cl ' + str(args.connectivity_level)
for func in args.sens_funcs:
   opt_args += ' -sf '
   opt_args += func
opt_args += ' < '
opt_args += bcsource
opt_args += ' > instrumented.bc'
# print opt_args
opt_call = subprocess.Popen(opt_args, stdout = subprocess.PIPE, shell=True)
out, err = opt_call.communicate()
print out

clang_call = subprocess.Popen(['clang-3.9', 'instrumented.bc', '-o', 'instrumented'], subprocess.PIPE)
out, err = clang_call.communicate()
#print out

#llc_call = subprocess.Popen(['llc-3.9', '-filetype=obj', './instrumented.bc'], stdout = subprocess.PIPE)
#out, err = llc_call.communicate()
#print out

#protect_to_obj_call = subprocess.Popen(['cc', '-c', 'protect.c'], stdout = subprocess.PIPE)
#out, err = protect_to_obj_call.communicate()
#print out

#link_all_call = subprocess.Popen(['cc', 'instrumented.o', 'protect.o'], stdout = subprocess.PIPE)
#out, err = link_all_call.communicate()
#print out

instrumented_call = subprocess.Popen(['./instrumented'], stdout = subprocess.PIPE)
out, err = instrumented_call.communicate()
print out

