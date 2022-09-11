import csv
import os
import sys
import matplotlib.pyplot as plt
import numpy as np

cache_size=8192
assoc=[4,8,16]
blocksize=64
tracefile = "canneal.04t.longTrace"
num_processor = 4
protocol = 1


i=0

#file =open("dummy_py.txt", "wb")
lines= []
values=[]
categaories=["reads","r_misses","writes","w_misses","miss_rate","writebacks","c2C","Memory","intervention","invalidations","flushes","BusRdx"]
#lines = file.readlines()

with open("Graph2_.csv", 'w')as csvfile:
    fieldnames =['Title', 'Value']
    filewriter=csv.DictWriter(csvfile, fieldnames=fieldnames)
    while(i<3):
        with open("dummy_py.txt","w") as file:
            file.write(str(assoc[i]))
            file.write("\n")
            cmd= r'./smp_cache'+ " "+ str(cache_size)+ " "+ str(assoc[i])+" "+ str(blocksize)+ " "+ str(num_processor)+ " "+ str(protocol) + " "+'trace/canneal.04t.longTrace'
            test = (os.popen(cmd).read())
            file.write(test)
            file.write("\n")
            s= test.replace("\n", ' ')
            #print(test)
        with open("dummy_py.txt", 'r')as f:
            lines = f.readlines()
            print(lines)
            matchline = ":"
            #studentline="ECE406"
            correct_line=1
            curent_cache=0
            current_cache_s=0

            for line in lines:
                if(matchline in line.strip() ):
                    #print(line)

                    newline = line.split(':')
                    temp1= newline[0].strip()
                    temp2= newline[-1].strip()
                    filewriter.writerow({'Title': temp1, 'Value': str(temp2)})
                    if(correct_line>=8):
                        values.append(int(float(temp2)))
                        curent_cache=curent_cache+1

                    if(curent_cache==12):
                        fig=plt.figure(figsize=(25,5))
                        current_cache_s=current_cache_s+1
                        plt.ylim(1,15000)

                        dim=np.arange(0,15000,500)
                        plt.yticks(dim)
                        #plt.hist(values)
                        plt.bar(categaories,values)
                        plt.text(0,3,"Cache_size"+str(int(8192/1000))+"MB",fontsize=10)
                        plt.text(5,3,"Block_Size "+ str(blocksize),fontsize=10)
                        plt.text(10,3,"Assoc "+ str(assoc[i]),fontsize=10)
                        if(protocol==0):
                            plt.text(12,3,"MSI",fontsize=10)
                        elif(protocol==1):
                            plt.text(12,3,"MESI",fontsize=10 )
                        else:
                            plt.text(12,3,"DRAGON",fontsize=10 )
                        plt.title("Cache_NO "+str(current_cache_s))
                        plt.savefig("plot/Assoc"+str(assoc[i])+"__"+str(current_cache_s)+".jpg")

                        curent_cache=0
                        del values[:]
                        plt.close()
                    correct_line=correct_line+1

                   #print(temp1)
                    #print(temp2)
        i+=1


#print("end")
    
