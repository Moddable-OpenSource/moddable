#!/bin/sh

url="https://www.unicode.org/Public/UCD/latest/ucd"

names="\
CaseFolding.txt \
CompositionExclusions.txt \
DerivedCoreProperties.txt \
NormalizationTest.txt \
SpecialCasing.txt \
UnicodeData.txt"

mkdir -p data

for name in $names; do
   curl ${url}/${name} -o data/${name}
done
