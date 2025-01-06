import ctypes
import sys
import os


# Get the command-line arguments passed to Python (sys.argv)
# sys.argv includes the script name as the first argument, just like in C
args = sys.argv

print(f"running program:{args[0]} dll : {args[1]}\n",flush=True)

dll_path = os.path.abspath(args[1])
print(f"Attempting to load DLL from: {dll_path}\n",flush=True)
# Load the shared library (DLL)

try:
    main_program = ctypes.CDLL(dll_path)
    print(f"Successfully loaded: {dll_path}\n",flush=True)
except OSError as e:
    print(f"Failed to load DLL: {dll_path}\nError: {e}\n",flush=True)

# Define the argument types and return type for the C main function
main_program.main.argtypes = (ctypes.c_int, ctypes.POINTER(ctypes.POINTER(ctypes.c_char)))
main_program.main.restype = ctypes.c_int

# Convert the Python list of arguments to a list of bytes (C strings)
args_c = [arg.encode('utf-8') for arg in args[1:]]

# Calculate argc (number of arguments)
argc = len(args_c)

print(f"running with {argc} arguments {args_c}\n",flush=True)

# Create argv: a ctypes array of char* (char pointers)
argv = (ctypes.POINTER(ctypes.c_char) * argc)(*map(ctypes.create_string_buffer, args_c))

# Call the C main function with argc and argv
main_program.main(argc, argv)
