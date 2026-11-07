# `cmacmd` --- C Macro Markdown

## Problems I faced
0. I did not want to quote everything but then
1. stuff like // did not work anymore
2. even commas would break macro expansion
3. quotes are not macro expanded, so had to make SQQ and DQQ
4. Special characters niladic entities
5. Stringizing collapses whitespace, so pre and code were particularly hard
6. also pre and code had to be able to use <> without issues, so I had to work around that or require the commands to be written with the entity names.
7. Stringizing introduces spaces around args if they were not there before.
8. empty lines needed special handling
9. inline code and code files 
10. macros can't enforce nesting
11. Content model/ validation of html


## TODO
0. attributes for tags
1. images
2. extension: minification
3. extension: zstd compression
4. extension: replacement
