import random

class Predictor:
    def __init__(self):
        pass
    def predict(self, open, high, low, close, volume):
        print(f'open={open}\nhigh={high}\nlow={low}\nclose={close}\nvolume={volume}')
        list_tag = ['enter_long', 'enter_short','exit_long','exit_short', '','','','']
        list_value = [0,1]
        local_dict = {random.choice(list_tag): random.choice(list_value)}
        print(local_dict)
        return local_dict

def predict(open, high, low, close, volume):
    print(f'open={open}\nhigh={high}\nlow={low}\nclose={close}\nvolume={volume}')
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