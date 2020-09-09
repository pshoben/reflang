#include "parser.class.hpp"

#include "parser.util.hpp"
#include "clang/AST/PrettyPrinter.h"

using namespace reflang;
using namespace std;

namespace
{
	Function GetMethodFromCursor(CXCursor cursor)
	{
		auto type = clang_getCursorType(cursor);

		Function f(
				parser::GetFile(cursor), parser::GetFullName(cursor));
		f.Name = parser::Convert(clang_getCursorSpelling(cursor));
		int num_args = clang_Cursor_getNumArguments(cursor);
		for (int i = 0; i < num_args; ++i)
		{
			auto arg_cursor = clang_Cursor_getArgument(cursor, i);
			NamedObject arg;
			arg.Name = parser::Convert(
					clang_getCursorSpelling(arg_cursor));
			if (arg.Name.empty())
			{
				arg.Name = "nameless";
			}
			auto arg_type = clang_getArgType(type, i);
			arg.Type = parser::GetName(arg_type);
			f.Arguments.push_back(arg);
		}

		f.ReturnType = parser::GetName(clang_getResultType(type));
		return f;
	}

	NamedObject GetFieldFromCursor(CXCursor cursor)
	{
		NamedObject field;
		field.Name = parser::Convert(clang_getCursorSpelling(cursor));
		field.Type = parser::GetName(clang_getCursorType(cursor));
		return field;
	}

	string GetBaseClassFromCursor(CXCursor cursor)
	{
		string base_class_name = clang_getCString(clang_getCursorSpelling(cursor));	
		return base_class_name.erase(0,6); // remove the "class " prefix of "class BaseType"
	}

	CXChildVisitResult VisitClass(
			CXCursor cursor, CXCursor parent, CXClientData client_data)
	{
		auto* c = reinterpret_cast<Class*>(client_data);

		if (clang_getCXXAccessSpecifier(cursor) == CX_CXXPublic)
		{
			switch (clang_getCursorKind(cursor))
			{
			case CXCursor_CXXMethod:
				if (clang_CXXMethod_isStatic(cursor) != 0)
				{
					c->StaticMethods.push_back(GetMethodFromCursor(cursor));
				}
				else
				{
					c->Methods.push_back(GetMethodFromCursor(cursor));
				}
				break;
			case CXCursor_FieldDecl:
				c->Fields.push_back(GetFieldFromCursor(cursor));
				break;
			case CXCursor_CXXBaseSpecifier:
			{
				string base_class_name = GetBaseClassFromCursor(cursor);
				c->BaseClasses.push_back( base_class_name );
				break;
			}
			case CXCursor_VarDecl:
				c->StaticFields.push_back(GetFieldFromCursor(cursor));
				break;
			default:
				break;
			}
		}
		return CXChildVisit_Continue;
	}
}

Class parser::GetClass(CXCursor cursor)
{
	Class c(GetFile(cursor), GetFullName(cursor));
	clang_visitChildren(cursor, VisitClass, &c);
	return c;
}
