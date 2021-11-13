-- Dump the table (t) to a string
function Serialize(t)
	local format_=string.format
	local concat_=table.concat
	local next_=next
	local type_=type
	local tostring_=tostring

	local obj={
		string=function(v) return format_("%q",v) end,
		number=tostring_,
		boolean=tostring_ 
	}
	obj.table=function(t,indent) --subtable, indentation depth
		local subindent,out,i=indent.."\t",{"{"},2
		for n,v in next_,t,nil do
			out[i]=
				subindent.."["
				..obj[type_(n)](n,subindent).."]="
				..obj[type_(v)](v,subindent)..","
			i=i+1
		end
		out[i]=indent.."}"
		return concat_(out,"\r\n")
	end
	return "return "..obj.table(t,"")
end

-- (c) 2018 Ian Underwood
-- All rights reserved.
-- 
-- Permission is hereby granted, free of charge, to any person obtaining a
-- copy of this software and associated documentation files (the
-- "Software"), to deal in the Software without restriction, including
-- without limitation the rights to use, copy, modify, merge, publish,
-- distribute, and/or sell copies of the Software, and to permit persons to
-- whom the Software is furnished to do so, provided that the above
-- copyright notice(s) and this permission notice appear in all copies of
-- the Software and that both the above copyright notice(s) and this
-- permission notice appear in supporting documentation.
-- 
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
-- OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
-- MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
-- THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
-- INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
-- OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
-- OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
-- OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
-- PERFORMANCE OF THIS SOFTWARE.

