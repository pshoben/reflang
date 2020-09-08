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

	NamedObject GetBaseClassFromCursor(CXCursor cursor)
	{
		NamedObject field;
		field.Name = clang_getCString(clang_getCursorSpelling(cursor));	
		printf("got base class %s\n",field.Name.c_str());
		//field.Type = parser::GetName(clang_getCursorType(cursor));
		return field;
	}

	CXChildVisitResult VisitClass(
			CXCursor cursor, CXCursor parent, CXClientData client_data)
	{
		auto* c = reinterpret_cast<Class*>(client_data);

		printf("VisitClass got CXXCursorKind(%d)\n",clang_getCursorKind(cursor));

		//if (clang_getCXXAccessSpecifier(cursor) == CX_CXXPublic)
		//{
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
				printf("VisitClass got CXXCursorKind(%d) CXCursor_FieldDecl \n",clang_getCursorKind(cursor));
				c->Fields.push_back(GetFieldFromCursor(cursor));
				break;
			case CXCursor_CXXBaseSpecifier:
			{
				printf("VisitClass got CXXCursorKind(%d) CXCursor_CXXBaseSpecifier \n",clang_getCursorKind(cursor));
//				clang::LangOptions lo;
//				struct clang::PrintingPolicy pol(lo);
//				pol.adjustForCPlusPlus();
//				pol.TerseOutput = true;
//				pol.FullyQualifiedName = true;
//				string rv = clang_getCursorPrettyPrinted(cursor, &pol);
				//printf("VisitClass got base specifier %s\n",rv);
				NamedObject no = GetBaseClassFromCursor(cursor);
				break;
			}
			case CXCursor_VarDecl:
				c->StaticFields.push_back(GetFieldFromCursor(cursor));
				break;
			default:
				break;
			}
		//}
		return CXChildVisit_Continue;
	}
}

Class parser::GetClass(CXCursor cursor)
{
	Class c(GetFile(cursor), GetFullName(cursor));
	clang_visitChildren(cursor, VisitClass, &c);
	return c;
}
