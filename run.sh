export 'NS_LOG=*=level_all|prefix_func|prefix_time'
#export 'NS_LOG=Node=level_all|prefix_func|prefix_time:Ipv4L3Protocol=level_all|prefix_func|prefix_time'
#export 'NS_LOG=Queue=level_all|prefix_func|prefix_time'


# big fat tree with standard workload
#./waf --run "wan-internet2-sack -topofile=topo-big-fattree.txt -endhostfile=endhosts-big-fattree.txt -workload=workload-big-fattree-load-70.txt"

# two to one setting
#export 'NS_LOG=Queue=level_all|prefix_func|prefix_time:DropTailQueue=level_all|prefix_func|prefix_time:TCPTopology=level_all|prefix_func|prefix_time'
./waf --run "wan-internet2-sack -topofile=scratch/topo-2-to-1.txt -endhostfile=scratch/endhosts-2-to-1.txt -workload=scratch/workload-2-to-1.txt"

#./waf --run wan-internet2-sack --command-template="gdb --args %s args"
#(gdb) r -topofile=topo-big-fattree.txt -endhostfile=endhosts-big-fattree.txt -workload=workload-big-fattree-load-70.txt

#(gdb) r -topofile=scratch/topo-2-to-1.txt -endhostfile=scratch/endhosts-2-to-1.txt -workload=scratch/workload-2-to-1.txt

# remove hexadecimal addresses in vim
# :%s/0x\w\+//g
