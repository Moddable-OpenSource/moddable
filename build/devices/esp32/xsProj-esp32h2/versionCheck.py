import sys

if len(sys.argv) < 3:
	print("Not enough parameters")
	sys.exit(1)

update = "  See update instructions at: https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/devices/esp32.md"
expected = sys.argv[1]
given = sys.argv[2]

g = given.split(".")
if len(g) < 3:
	g.append(0)
e = expected.split(".")
if len(e) < 3:
	e.append(0)

if g[0] == e[0]:
	if g[1] == e[1]:
		if g[2] == e[2]:
			print("Using recommended ESP-IDF " + given)
			sys.exit(0)
		else:
			print("Recommend using ESP-IDF " + expected + " (found " + given + ")")
			print(update)
			sys.exit(0);
	else:
		print("*** Update required to ESP-IDF " + expected + " (found " + given + ")")
		print(update)
		sys.exit(1);
else:
	print("*** Update required to ESP-IDF " + expected + " (found " + given + ")")
	print(update)
	sys.exit(1);
