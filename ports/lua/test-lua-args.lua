-- test-lua-args.lua: verify the arg table is populated correctly
-- arg[0] is the script name, arg[-1] and below are interpreter options
-- positive indices are script arguments
print(type(arg))
print(arg[0])
