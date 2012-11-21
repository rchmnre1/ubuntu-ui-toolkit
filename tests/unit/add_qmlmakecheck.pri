# Do not use CONFIG += testcase that would add a 'make check' because it also
# adds a 'make install' that installs the test cases, which we do not want.
# Instead add a 'make check' manually.

check.target = check
check.commands = ./$$TARGET -platform minimal
check.commands += -import \"../../../modules\"
check.commands += -maxwarnings 4 -xunitxml >
check.commands += ../../test_$(TARGET).xml