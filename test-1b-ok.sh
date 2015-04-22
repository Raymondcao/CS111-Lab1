#! /bin/sh

cat >test.sh <<'EOF'

#simple command
touch file

rm file

echo Pass simple command test

#AND command
touch fileAnd && rm fileAnd

touch shouldNotExistAND.txt

rm shouldNotExistAND.txt && echo Pass And command test

#OR command
echo No file should be created || touch ShouldNotExistOR.txt

echo Pass Or command test

#PIPE command
touch PassPIPE

ls | grep PassP

ls | grep Make

rm PassPIPE

#Redirection
ls -al> redir.txt

cat <redir.txt >redirResult.txt

diff redir.txt redirResult.txt

rm redir.txt

rm redirResult.txt

echo Pass redir


#Subshell
(echo Subshell Test> subshell.txt)

(cat subshell.txt)

(rm subshell.txt)

echo Pass subshell

EOF


./timetrash test.sh || exit
rm test.sh
