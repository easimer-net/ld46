if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syn include @cppSignature syntax/cpp.vim
syn keyword entgenTableKeyword table nextgroup=entgenTableId skipwhite
syn keyword entgenAliasKeyword alias nextgroup=entgenTableId skipwhite
syn keyword entgenInterfaceKeyword interface nextgroup=entgenTableId skipwhite
syn keyword entgenBuiltinTypes bool vec4 float int char
syn region entgenTable start="{" end="}" fold transparent
syn match entgenDirective '\#\w\+'
syn match entgenDirectiveWithParam '\#\w\+(' nextgroup=entgenDirectiveParam skipwhite
syn match entgenDirectiveParam '\i\+' nextgroup=entgenDirectiveWithParamEnd skipwhite
syn match entgenDirectiveWithParamEnd ')'
syn match entgenArrayType '\w\+\[\d\+\]'
syn match entgenType '\(\:\s*\)\(\w\+\)\(\;\)'
syn match entgenTableId '\i\+' nextgroup=entgenTableVarName skipwhite
syn match entgenTableVarName '\i\+'

syn keyword entgenMemberFunction member_function nextgroup=entgenCppSignature skipwhite
syn region entgenCppSignature start="'" end="'" contains=@cppSignature

syn keyword entgenIncludeKeyword incldude nextgroup=entgenPath skipwhite
syn region entgenInclude start="'" end="'"

hi def link entgenTableKeyword Keyword
hi def link entgenAliasKeyword Keyword
hi def link entgenInterfaceKeyword Keyword

hi def link entgenDirective PreProc
hi def link entgenDirectiveWithParam PreProc
hi def link entgenDirectiveParam Underlined
hi def link entgenDirectiveWithParamEnd PreProc

hi def link entgenBuiltinTypes Type
hi def link entgenArrayType Type
hi def link entgenType Type
hi def link entgenTableId Underlined
hi def link entgenTableVarName Identifier

hi def link entgenMemberFunction Keyword

hi def link entgenIncludeKeyword Keyword
hi def link entgenInclude String

let b:current_syntax = "entity_gen"
