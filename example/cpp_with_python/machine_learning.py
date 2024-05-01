import random

def predict(open, high, low, close, volume):
    list_tag = ['enter_long', 'enter_short','exit_long','exit_short', '','','','']
    list_value = [0,1]
    local_dict = {random.choice(list_tag): random.choice(list_value)}
    return local_dict

def main():
    predict(1,1,1,1,1)
    

if __name__ == "__main__":
    main()

def multiply(a,b):
    print("Will compute", a, "times", b)
    c = 0
    for i in range(0, a):
        c = c + b
    return c