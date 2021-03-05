Convert literal Python to Pickle.

Why:

`ast.literal_eval` is very slow.
`json.loads` shows that it can be much faster.
`pickle.loads` is yet faster.

