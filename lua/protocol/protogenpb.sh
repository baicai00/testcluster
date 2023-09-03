#!/bin/sh

if [ -z "$1" ] ; then
	echo usage:
	echo ./protogenpb.sh PUB_IMPORT_DIR
	exit
fi

INNER_IMPORT="./"
PUB_IMPORT="$1"

protofiles=`ls $INNER_IMPORT/*.proto $PUB_IMPORT/*.proto`

rm -rf *.pb  ./pb/*.pb

bin=/usr/local/protobuf34/bin/protoc

for f in ${protofiles[*]}
do
    b=${f/"$1"/"."}
	$bin $f -o "pb/"${b/".proto"/".pb"} 2>&1 -I"$INNER_IMPORT" -I"$PUB_IMPORT"
done