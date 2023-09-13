# SHV type-info grammar description

_TypeInfo_ =\
&nbsp;&nbsp;`<"version": 4> {`\
&nbsp;&nbsp;`"devicePaths"`: `{`\
&nbsp;&nbsp;&nbsp;&nbsp;_DevicePath_: _DeviceName_\* \
&nbsp;&nbsp;`}`\
&nbsp;&nbsp;`"deviceDescriptions"`: `{`\
&nbsp;&nbsp;&nbsp;&nbsp;_DeviceName_: _DeviceDescription_\*\
&nbsp;&nbsp;`}`\
&nbsp;&nbsp;`"types": {`\
&nbsp;&nbsp;&nbsp;&nbsp;_DefinedTypeName_: _TypeDescription_\*\
&nbsp;&nbsp;`}`\
`}`

_DevicePath_ = _String_

_DeviceName_ = _String_
 
_DevicePath_ = `{`
&nbsp;&nbsp;`"properties"`: `[`
&nbsp;&nbsp;&nbsp;&nbsp;_PropertyDescription_\*
&nbsp;&nbsp;`]`?
`}`

_DeviceDescription_ = `{`\
&nbsp;&nbsp;`"properties"`: `[`\
_&nbsp;&nbsp;&nbsp;&nbsp;PropertyDescription_\*\
&nbsp;&nbsp;`]`?\
`}`

_PropertyDescription_ = `{`\
&nbsp;&nbsp;`"name"`: String\
&nbsp;&nbsp;`"typeName"`: _TypeName_\
&nbsp;&nbsp;`"label"`: _String_?\
&nbsp;&nbsp;`"description"`: _String_?\
&nbsp;&nbsp;`"methods"`: `[`\
&nbsp;&nbsp;&nbsp;&nbsp;_MethodDescription_\*\
&nbsp;&nbsp;`]`?\
&nbsp;&nbsp;`"autoload"`: _Bool_?\
&nbsp;&nbsp;`"monitored"`: _Bool_?\
`}`

_MethodDescription_ =  `{`\
&nbsp;&nbsp;`"name"`: String\
&nbsp;&nbsp;`"accessGrant"`: (`"bws"` | `"rd"` | `"wr"` | `"cmd"` | `"cfg"` | `"svc"` | `"ssvc"` | `"dev"` | `"su"`)\
&nbsp;&nbsp;`"flags"`: _MethodFlags_\
&nbsp;&nbsp;`"signature"`: _MethodSignature_\
&nbsp;&nbsp;`"description"`: _String_?\
&nbsp;&nbsp;`"tags"`: `{`\
&nbsp;&nbsp;&nbsp;&nbsp;_TagName_: _Value_\*\
&nbsp;&nbsp;`}`?\
`}`

_MethodFlags_ = bitfield \
&nbsp;&nbsp;`Signal` = 1\
&nbsp;&nbsp;`Getter` = 2\
&nbsp;&nbsp;`Setter` = 4\
&nbsp;&nbsp;`LargeResultHint` = 8

_MethodSignature_ = enum \
&nbsp;&nbsp;`VoidVoid` = 0\
&nbsp;&nbsp;`VoidParam` = 1\
&nbsp;&nbsp;`RetVoid` = 2\
&nbsp;&nbsp;`RetParam` = 3

_TypeName_ = _String_\
  (_PrimitiveTypeName_ | _DefinedTypeName_)

_TypeDescription_ = `{`\
&nbsp;&nbsp;`"typeName"`: (_TypeName_ | `"Enum"` | `"BitField"`)\
&nbsp;&nbsp;`"fields"`: [\
&nbsp;&nbsp;&nbsp;&nbsp;_FieldDescription_\*\
&nbsp;&nbsp;`]`? // only for Enum and BitField types\
`}`

_FieldDescription_ = `{`\
&nbsp;&nbsp;`"name"`: String\
&nbsp;&nbsp;`"typeName"`: _TypeName_? // default is Bool\
&nbsp;&nbsp;`"label"`: _String_?\
&nbsp;&nbsp;`"description"`: _String_?\
&nbsp;&nbsp;`"value"`: (_EnumFieldValue_ | _BitFieldFieldIntValue_ | _BitFieldFieldRangeValue_)\
`}`

_EnumFieldValue_ = _Int_

_BitFieldFieldIntValue_ = _Int_ // bit index

_BitFieldFieldRangeValue_ = `[`\
&nbsp;&nbsp;_StartIndex_ = Int // index of LSB\
&nbsp;&nbsp;_EndIndex_ = Int   // index of MSB\
`]`