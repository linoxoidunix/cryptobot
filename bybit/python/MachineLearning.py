import random

def predict(open, high, low, close, volume):
    list_tag = ['enter_long', 'enter_short','exit_long','exit_short', '','','','']
    list_value = [0,1]
    local_dict = {random.choice(list_tag): random.choice(list_value)}
    print(local_dict)
    return local_dict

def main():
    predict(1,1,1,1,1)
    

if __name__ == "__main__":
    main()

def multiply():
    c = 12345*6789
    print ('The result of 12345 x 6789 :', c)
    return c