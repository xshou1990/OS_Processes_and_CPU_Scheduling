

import sys
def check_file(gt_f, our_f):
    gt = open(gt_f, "r") # ground truth
    our = open(our_f, "r")
    lines = gt.readlines()
    gt_lines = []
    for line in lines:
        gt_lines.append(line)
    our_lines = []
    lines = our.readlines()
    for line in lines:
        our_lines.append(line)

    #assert(len(gt_lines) == len(our_lines))
    print("len(gt_lines) =", len(gt_lines), "len(our_lines) =", len(our_lines))
    err = 0
    for i in range(len(gt_lines)):
        if i == len(our_lines):
            break
        if gt_lines[i] != our_lines[i]:
            err += 1
            if i > 2694:
                print(i)
                print("gt: ", gt_lines[i])
                print("our:", our_lines[i])
                #exit()
        
    if err == 0:
        print("SUCCESS!!!!!  ><")
    # Close the file
    gt.close()
    our.close()

if __name__ == "__main__":
    test_case = int(sys.argv[1])
    if test_case == 0:
        print("Test case 1: ./a.out 3 1 1024 0.001 3000 4 0.75 256")
        print("==> Checking std output")
        check_file("gt.txt", "log.txt")
        print("==> Checking simout.txt")
        check_file("gt_simout.txt", "simout.txt")
    elif test_case == 1:
        print("Test case 2: ./a.out 8 6 512 0.001 3000 4 0.5 32")
        print("==> Checking std output")
        check_file("gt1.txt", "log1.txt")
        print("==> Checking simout.txt")
        check_file("gt_simout1.txt", "simout.txt")
    elif test_case == 2:
        print("Test case 3: ./a.out 16 2 256 0.001 3000 4 0.5 32")
        print("==> Checking std output")
        check_file("gt2.txt", "log2.txt")
        print("==> Checking simout.txt")
        check_file("gt_simout2.txt", "simout.txt")