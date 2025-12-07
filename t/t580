\l base64

/ write out a binary testfile
tf:"tf",($.z.P),$.z.t
a:bd draw[_1e3;0]
/tf 6:a

e:base64.e a
d:base64.d e
a~d / original matches decoded?

base64.e""
base64.d base64.e""

base64.e"charles"
base64.d base64.e"charles"

base64.e"mary had a little lamb, its fleece was white as snow. and everyhwere that mary went, ..."
base64.d base64.e"mary had a little lamb, its fleece was white as snow. and everyhwere that mary went, ..."

/ compare to base64 program
/(tf,"e")6:e
/."\\base64 -d ",tf,"e > ",tf,"d"
/p:6:tf,"d"
/a~p

/del tf
/del tf,"e"
/del tf,"d"

\\
