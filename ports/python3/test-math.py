import sys
print(sys.version)
print(sys.platform)
print(2 + 2)
print(3.14 * 2)

for i in range(5):
    print(i, end=" ")
print()

def greet(name):
    return "Hello, " + name + "!"

print(greet("Amiga"))
