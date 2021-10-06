
import sys

if len(sys.argv) < 3:
	print("Not enough parameters")
	sys.exit(1)

given = sys.argv[1]
expected = sys.argv[2]

g = given.split(".")
if len(g) < 3:
	g.append(0)
e = expected.split(".")
if len(e) < 3:
	e.append(0)

if g[0] == e[0]:
	if g[1] == e[1]:
		if g[2] == e[2]:
#			print("versions are the same.")
			sys.exit(0)
		else:
#			print("major and minor versions are the same, it should work.")
			sys.exit(0);
	else:
#		print("minor versions differ. Please update.");
		sys.exit(1);
else:
#	print("major versions differ. Please update.");
	sys.exit(1);

