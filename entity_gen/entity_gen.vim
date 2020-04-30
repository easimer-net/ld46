if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syn keyword entgenKeywords table nextgroup=entgenTableId skipwhite
syn keyword entgenBuiltinTypes bool vec4 float int char
syn region entgenTable start="{" end="}" fold transparent
syn match entgenDirective '\#\w\+'
syn match entgenArrayType '\w\+\[\d\+\]'
syn match entgenType '\(\:\s*\)\(\w\+\)\(\;\)'
syn match entgenTableId '\i\+' nextgroup=entgenTableVarName skipwhite
syn match entgenTableVarName '\i\+'

hi def link entgenKeywords Keyword
hi def link entgenDirective PreProc
hi def link entgenBuiltinTypes Type
hi def link entgenArrayType Type
hi def link entgenType Type
hi def link entgenTableId Underlined
hi def link entgenTableVarName Identifier

let b:current_syntax = "entity_gen"
