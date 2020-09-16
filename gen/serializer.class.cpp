#include "serializer.class.hpp"

#include <map>
#include <sstream>
#include <vector>
#include <set>
#include <memory>
#include <cstddef>
#include <string>

#include "serializer.function.hpp"
#include "serializer.util.hpp"
#include "tests/catch.hpp"

using namespace std;
using namespace reflang;

namespace
{
	string IterateFields(const Class& c)
	{
		stringstream tmpl;

		for (const auto& field : c.Fields)
		{
			tmpl << "	t(c." << field.Name << ");\n";
		}

		return tmpl.str();
	}

	vector<string> split( string str, char delim ) 
	{
		std::istringstream ss( str );
		vector<string> out;
		string token;
		while( std::getline( ss,token,delim )) {
			out.push_back( token ) ;
		}
		return out;
	}

	bool IsFundamentalType(string base)
	{
		static set<string> fundamentals = { "int", "long", "char", "double", "float" } ;
		auto it = fundamentals.find(base);
		return it != fundamentals.end();
	}

	string GetBaseType(string type) 
	{
		// TODO cv-qualifiers etc
		vector<string> arr = split( type, ' ' );
		string first = arr[0];
		if( !first.compare("const")) {
			if (arr.size() > 1) {
				return arr[1];
			}
		} else {	
			return arr[0];	
		}
		return string("unknown");
	}
	bool IsPointerType(string type) 
	{
		vector<string> arr = split( type, ' ' );
		for( string s : arr ) {
			if( s.rfind( "*",0 )==0 )
				return true;
		}
		return false;
	}
	bool IsRefType(string type) 
	{
		vector<string> arr = split( type, ' ' );
		for( string s : arr ) {
			if( s.rfind( "&",0 )==0 )
				return true;
		}
		return false;
	}
	bool IsArrayType(string type) 
	{
		size_t index = type.find( "[" );
		if( index != string::npos ) 
			return true;
		return false;
	}
	bool GetArrayRank(string type) 
	{
		return 1 ; // TODO
	}
	int GetArraySize(string type) 
	{
		vector<string> arr = split( type, ' ' );
		for( string s : arr ) {
			size_t index = s.rfind( "[" ) ;
			if( index != string::npos ) {
				string trimmed = s.substr( index+1,s.size()-(index+2) ); // remove the surrounding [ ] 
				return std::stoi(trimmed);
			}
		}
		// TODO not found
		return 0;
	}

	const TypeBase * findType(const string & name, const std::vector<std::unique_ptr<TypeBase>>& types)
	{	
		for( const auto& t : types ) {
			if( !name.compare(t->GetFullName())) {
				return t.get();
			}
		}
		return NULL;
	}
	
	string IterateFieldsAndValues(const Class& c, const std::vector<std::unique_ptr<TypeBase>>& types, string indent, string var_name, bool is_pointer)
	{
		stringstream tmpl;
		int field_count = 0;
		string redirect_parent = (is_pointer) ? "->" : ".";

		for (const auto& base_class_name : c.BaseClasses) {
			const Class * baseType = dynamic_cast<const Class*>(findType( base_class_name, types ));
			if( baseType ) {
				tmpl << "	t(\"" << indent
				<< "base class " << base_class_name << ":\",\"\");\n";
				tmpl << IterateFieldsAndValues( *baseType, types, 
								indent + "  ", var_name,
								false );
			}
		}

		for (const auto& field : c.Fields) {
		
			string base = GetBaseType(field.Type);
			string redirect_field = "";
			//string subtype_redirect = ".";
			if( IsPointerType( field.Type )) {
				redirect_field = "*";
				//subtype_redirect = "->";
			}
			if( IsFundamentalType( base ))  {
				if( IsArrayType( field.Type ) 
				&& strcmp( base.c_str(), "char" )) { // not a char array
				
					tmpl << "	t(\"" << indent  // <<  field.Type << " " 
					<< field.Name << ":\",\"\");\n";

					int arraySize = GetArraySize( field.Type );
					for( int i = 0 ; i < arraySize ; ++i ) {
						tmpl << "	t(\"" << indent << "  - " // << field.Type << " " 
						"\", " << redirect_field << var_name << redirect_parent << field.Name << "[" << i << "]);\n";
					}
				} else {
					tmpl << "	t(\"" << indent  // <<  field.Type << " " 
					<< field.Name << ": \", " << redirect_field << var_name << redirect_parent << field.Name << ");\n";
				}
			} else {
				const Class * subType = dynamic_cast<const Class*>(findType( base, types )) ;
				if( subType ) {
					tmpl << "	t(\"" << indent  // <<  field.Type << " " 
					<< field.Name << ":\",\"\");\n";
					if( IsArrayType( field.Type )) { 
						int arraySize = GetArraySize( field.Type );
						for( int i = 0 ; i < arraySize ; ++i ) {
							string subfield_name = var_name + redirect_parent + field.Name + "[" + to_string(i) + "]";
							if( IsPointerType( field.Type )) {
								tmpl << " if( " <<  subfield_name  << ") {\n";
							}
							tmpl << IterateFieldsAndValues( *subType, types, indent + "  - ", subfield_name ,
							IsPointerType( field.Type )) ;

							if( IsPointerType( field.Type )) {
								tmpl << " } else { t( " + subfield_name + ", \"null\");}\n";
							}

						}

					} else {
						string subfield_name = var_name + redirect_parent + field.Name;
						if( IsPointerType( field.Type )) {
							tmpl << " if( " <<  subfield_name  << ") {\n";
						}
						tmpl << IterateFieldsAndValues( *subType, types, 
										indent + "  ", subfield_name,
										IsPointerType( field.Type )) ;
						if( IsPointerType( field.Type )) {
							tmpl << " } else { t( " + subfield_name + ", \"null\");}\n";
						}

					}
				}
			}
			// yaml syntax requires list entries that are structs to be indented, and marked with "-" (but only on the first fielD) 
			if( field_count == 0 ) {
				size_t index = 0;
				while( true ) {
					index = indent.find( "-",index );
					if( index == string::npos ) 
						break;
					indent.replace( index,1," " );
					++index;
				}	
			}
			field_count++;
		}
		return tmpl.str();
	}

//	string get_base_type(string type_name) {
//		printf("GetBaseType(%s) > (%s)\n", type_name.c_str(), type_name.c_str());
//		return type_name;	
//	}

	string print_class_yaml(const Class& c, const std::vector<std::unique_ptr<TypeBase>>& types,  string indent, string var_name )
	{
		stringstream tmpl;
		int field_count = 0;
		//string "->" = "->";

		bool found_base_class = false;
		for (const auto& base_class_name : c.BaseClasses) {
			const Class * baseType = dynamic_cast<const Class*>(findType( base_class_name, types ));
			if( baseType ) {

				found_base_class = true;
				tmpl << "	lprint( indent, \""
				<< "base class " << base_class_name << ":\",\"\" );\n";
				tmpl << "	Class<" << base_class_name << ">::print_class_yaml( static_cast<const " << base_class_name << " * >(c), indent + \"    \", lprint );\n" ;
			}
		}
		if( found_base_class ) {
			tmpl << "\n";
		}

		for (const auto& field : c.Fields) {
		
			string base = GetBaseType(field.Type);
			if( IsFundamentalType( base ))  {
				if( IsArrayType( field.Type ) 
				&& strcmp( base.c_str(), "char" )) { // not a char array
				
					tmpl << "	lprint( indent, \""
					<< field.Name << ":\",\"\" );\n";

					int arraySize = GetArraySize( field.Type );
					for( int i = 0 ; i < arraySize ; ++i ) {
						if( IsPointerType( field.Type )) {
							tmpl << "	if( " <<  var_name << "->" << field.Name  << "[" << i << "]) {\n";
							tmpl << "		lprint( indent + \"  - \", \"\", *(" << var_name << "->" << field.Name << "[" << i << "]));\n"; 
							tmpl << "	} else {\n";
							tmpl << "		lprint( indent + \"  - \", \"\", \"null\" );\n";
							tmpl << "	}\n";
						} else {
							tmpl << "	lprint( indent +  \"  - \", \"\", " << var_name << "->" << field.Name << "[" << i << "]);\n"; 
						}	
					}
				} else {
					if( IsPointerType( field.Type )) {
						tmpl << "	if( " <<  var_name << "->" << field.Name << ") {\n";
						tmpl << "		lprint( indent, \"" << field.Name << ": \", *(" << var_name << "->" << field.Name << "));\n";
						tmpl << "	} else {\n";
						tmpl << "		lprint( indent, \"" << field.Name << ":\", \"null\" );\n";
						tmpl << "	}\n";

					} else {
						tmpl << "	lprint( indent, \"" 
						<< field.Name << ": \", " << var_name << "->" << field.Name << ");\n";
					}	
				}
			} else {
				const Class * subType = dynamic_cast<const Class*>(findType( base, types )) ;
				if( subType ) {
					tmpl << "\n";
					tmpl << "	lprint( indent, \"" << field.Name << ":\",\"\" );\n";

					if( IsArrayType( field.Type )) { 
						int arraySize = GetArraySize( field.Type );
						for( int i = 0 ; i < arraySize ; ++i ) {

							string subfield_name = var_name + "->" + field.Name + "[" + to_string(i) + "]";
							tmpl << "	lprint( indent, \"  - " << field.Name << "_" << to_string(i) << ":\",\"\" );\n";
	
							if( IsPointerType( field.Type )) {

								tmpl << "	if( " <<  subfield_name  << " ) {\n";
						
								tmpl << "		Class<" << GetBaseType(field.Type) << ">::print_class_yaml(" 
								<< " " << subfield_name << ", indent + \"    \", lprint );\n" ;

								tmpl << "	} else {\n";
								tmpl << "		lprint( indent + \"    \", \"" << field.Name << "_" << to_string(i) << ":\", \"null\" );\n";
								tmpl << "	}\n";

							} else {

								tmpl << "	Class<" << GetBaseType(field.Type) << ">::print_class_yaml(" 
								<< " &(" << subfield_name << "), indent + \"        \", lprint );\n" ;
							}
						}
					} else {
						string subfield_name = var_name + "->" + field.Name;

						if( IsPointerType( field.Type )) {
							tmpl << "	if( " <<  subfield_name  << " ) {\n";
						
							tmpl << "		Class<" << GetBaseType(field.Type) << ">::print_class_yaml(" 
							<< " " << subfield_name << ", indent + \"    \", lprint );\n" ;

							tmpl << "	} else {\n";
							tmpl << "		lprint( indent + \"    \", \"" + field.Name << ":\", \"null\" );\n";
							tmpl << "	}\n";
						} else {
							tmpl << "	Class<" << GetBaseType(field.Type) << ">::print_class_yaml(" 
							<< " &(" << subfield_name << "), indent + \"    \", lprint );\n" ;
						}
					}
				}
			}
			// yaml syntax requires list entries that are structs to be indented, and marked with "-" (but only on the first fielD) 
			if( field_count == 0 ) {
				size_t index = 0;
				while( true ) {
					index = indent.find( "-",index );
					if( index == string::npos ) 
						break;
					indent.replace( index,1," " );
					++index;
				}	
			}
			field_count++;
		}
		return tmpl.str();
	}

	string read_class_yaml( Class& c, const std::vector<std::unique_ptr<TypeBase>>& types,  string indent, string var_name )
	{
		stringstream tmpl;
		int field_count = 0;

		tmpl << "	char dummy[1024];\n"; // lread any dummy values into here

		bool found_base_class = false;
		for (const auto& base_class_name : c.BaseClasses) {
			const Class * baseType = dynamic_cast<const Class*>(findType( base_class_name, types ));
			if( baseType ) {

				found_base_class = true;
				tmpl << "	lread( indent, \"" << "base class " << base_class_name << ":\", dummy);\n";
				tmpl << "	Class<" << base_class_name << ">::read_class_yaml( static_cast< " << base_class_name << " * >(c), indent + \"    \", lread );\n" ;
			}
		}
		if( found_base_class ) {
			tmpl << "\n";
		}

		for (const auto& field : c.Fields) {
		
			string base = GetBaseType(field.Type);

			if( IsFundamentalType( base ))  {
				if( IsArrayType( field.Type ) 
				&& strcmp( base.c_str(), "char" )) { // not a char array

					tmpl << "	lread( indent, \"" << field.Name << ":\", dummy );\n";

					int arraySize = GetArraySize( field.Type );
					for( int i = 0 ; i < arraySize ; ++i ) {
						if( IsPointerType( field.Type )) {
							tmpl << "	if( " <<  var_name << "->" << field.Name  << "[" << i << "]) {\n";
							tmpl << "		lread( indent + \"  - \", \"\", *(" << var_name << "->" << field.Name << "[" << i << "]));\n"; 
							tmpl << "	} else {\n";
							tmpl << "		lread( indent + \"  - \", \"\", dummy );\n";
							tmpl << "	}\n";
						} else {
							tmpl << "	lread( indent +  \"  - \", \"\", " << var_name << "->" << field.Name << "[" << i << "]);\n"; 
						}	
					}
				} else {
					if( IsPointerType( field.Type )) {
						tmpl << "	if( " <<  var_name << "->" << field.Name << ") {\n";
						tmpl << "		lread( indent, \"" << field.Name << ": \", *(" << var_name << "->" << field.Name << "));\n";
						tmpl << "	} else {\n";
						tmpl << "		lread( indent, \"" << field.Name << ":\", dummy );\n";
						tmpl << "	}\n";

					} else {
						tmpl << "	lread( indent, \"" << field.Name << ": \", " << var_name << "->" << field.Name << ");\n";
					}	
				}
			} else {

				const Class * subType = dynamic_cast<const Class*>(findType( base, types )) ;
				if( subType ) {
					tmpl << "\n";
					tmpl << "	lread( indent, \"" << field.Name << ":\", dummy);\n";

					if( IsArrayType( field.Type )) { 
						int arraySize = GetArraySize( field.Type );
						for( int i = 0 ; i < arraySize ; ++i ) {

							string subfield_name = var_name + "->" + field.Name + "[" + to_string(i) + "]";
							tmpl << "	lread( indent, \"  - " << field.Name << "_" << to_string(i) << ":\",dummy);\n";
	
							if( IsPointerType( field.Type )) {

								tmpl << "	if( " <<  subfield_name  << " ) {\n";
						
								tmpl << "		Class<" << GetBaseType(field.Type) << ">::read_class_yaml(" 
								<< " " << subfield_name << ", indent + \"    \", lread );\n" ;

								tmpl << "	} else {\n";
								tmpl << "		lread( indent + \"    \", \"" << field.Name << "_" << to_string(i) << ":\", dummy);\n";
								tmpl << "	}\n";

							} else {

								tmpl << "	Class<" << GetBaseType(field.Type) << ">::read_class_yaml(" 
								<< " &(" << subfield_name << "), indent + \"        \", lread );\n" ;
							}
						}
					} else {
						string subfield_name = var_name + "->" + field.Name;

						if( IsPointerType( field.Type )) {
							tmpl << "	if( " <<  subfield_name  << " ) {\n";
						
							tmpl << "		Class<" << GetBaseType(field.Type) << ">::read_class_yaml(" 
							<< " " << subfield_name << ", indent + \"    \", lread );\n" ;

							tmpl << "	} else {\n";
							tmpl << "		lread( indent + \"    \", \"" + field.Name << ":\", dummy);\n";
							tmpl << "	}\n";
						} else {
							tmpl << "	Class<" << GetBaseType(field.Type) << ">::read_class_yaml(" 
							<< " &(" << subfield_name << "), indent + \"    \", lread );\n" ;
						}
					}
				}
			}
			// yaml syntax requires list entries that are structs to be indented, and marked with "-" (but only on the first fielD) 
			if( field_count == 0 ) {
				size_t index = 0;
				while( true ) {
					index = indent.find( "-",index );
					if( index == string::npos ) 
						break;
					indent.replace( index,1," " );
					++index;
				}	
			}
			field_count++;
		}
		return tmpl.str();
	}


	string IterateStaticFields(const Class& c)
	{
		stringstream tmpl;

		for (const auto& field : c.StaticFields)
		{
			tmpl << "	t(" << c.GetFullName() << "::" << field.Name << ");\n";
		}

		return tmpl.str();
	}

	string MethodDeclaration(const Class& c, const Function& m)
	{
		stringstream tmpl;
		tmpl << R"(template <>
class Method<decltype(%name%), %name%> : public IMethod
{
public:
	const std::string& GetName() const override;
	int GetParameterCount() const override;
	Object Invoke(const Reference& o, const std::vector<Object>& args) override;
};

)";
		return serializer::ReplaceAll(
				tmpl.str(),
				{
					{"%name%", "&" + c.GetFullName() + "::" + m.Name},
				});
	}

	string MethodsDeclarations(const Class& c)
	{
		if (c.Methods.empty())
		{
			return string();
		}

		stringstream tmpl;
		tmpl << "// " << c.GetFullName() << " methods metadata.\n";

		for (const auto& method : c.Methods)
		{
			tmpl << MethodDeclaration(c, method);
		}

		tmpl << "// End of " << c.GetFullName() << " methods metadata.\n";

		return tmpl.str();
	}

	string StaticMethodsDeclarations(const Class& c)
	{
		if (c.StaticMethods.empty())
		{
			return string();
		}

		stringstream tmpl;
		tmpl << "// " << c.GetFullName() << " static methods metadata.\n";

		for (const auto& method : c.StaticMethods)
		{
			serializer::SerializeFunctionHeader(tmpl, method);
		}

		tmpl << "// End of " << c.GetFullName() << " static methods metadata.\n";

		return tmpl.str();
	}

	string GetCallArgs(const Function& m)
	{
		stringstream tmpl;
		for (size_t i = 0; i < m.Arguments.size(); ++i)
		{
			tmpl << "args[" << i << "].GetT<std::decay_t<"
				<< m.Arguments[i].Type << ">>()";
			if (i != m.Arguments.size() - 1)
			{
				tmpl << ", ";
			}
		}
		return tmpl.str();
	}

	string MethodDefinition(const Class& c, const Function& m)
	{
		stringstream tmpl;
		tmpl << R"(static std::string %escaped_name%_name = "%name%";

const std::string& Method<decltype(%pointer%), %pointer%>::GetName() const
{
	return %escaped_name%_name;
}

int Method<decltype(%pointer%), %pointer%>::GetParameterCount() const
{
	return %param_count%;
}

Object Method<decltype(%pointer%), %pointer%>::Invoke(
		const Reference& o, const std::vector<Object>& args)
{
	if (args.size() != %param_count%)
	{
		throw Exception("Invoke(): bad argument count.");
	}
)";
		if (m.ReturnType == "void")
		{
			tmpl << R"(	((o.GetT<%class_name%>()).*(%pointer%))(%call_args%);
	return Object();
)";
		}
		else
		{
			tmpl << R"(	return Object(((o.GetT<%class_name%>()).*(%pointer%))(%call_args%));
)";
		}
		tmpl << R"(}

)";
		return serializer::ReplaceAll(
			tmpl.str(),
			{
				{"%class_name%", c.GetFullName()},
				{"%pointer%", "&" + c.GetFullName() + "::" + m.Name},
				{"%name%", m.Name},
				{
					"%escaped_name%",
					serializer::GetNameWithoutColons(
							c.GetFullName()) + "_" + m.Name
				},
				{"%param_count%", to_string(m.Arguments.size())},
				{"%call_args%", GetCallArgs(m)}
			});
	}

	string MethodsDefinitions(const Class& c)
	{
		if (c.Methods.empty())
		{
			return string();
		}

		stringstream tmpl;
		tmpl << "// " << c.GetFullName() << " methods definitions.\n";

		for (const auto& method : c.Methods)
		{
			tmpl << MethodDefinition(c, method);
		}

		tmpl << "// End of " << c.GetFullName() << " methods definitions.\n";

		return tmpl.str();
	}

	map<string, vector<Function>> GetMethodsByName(
			const Class::MethodList& methods)
	{
		map<string, vector<Function>> methods_by_name;
		for (const auto& method : methods)
		{
			methods_by_name[method.Name].push_back(method);
		}
		return methods_by_name;
	}

	string GetMethodImpl(const Class& c)
	{
		map<string, vector<Function>> methods_by_name = GetMethodsByName(
				c.Methods);

		stringstream tmpl;
		bool first = true;
		for (const auto& methods : methods_by_name)
		{
			tmpl << "	";
			if (first)
			{
				first = false;
			}
			else
			{
				tmpl << "else ";
			}
			tmpl << "if (name == \"" << methods.first << "\")\n";
			tmpl << "	{\n";
			for (const auto& method : methods.second)
			{
				string name = "&" + c.GetFullName() + "::" + methods.first;
				tmpl << "		results.push_back(std::make_unique<Method<decltype("
					<< name << "), " << name << ">>());\n";
			}
			tmpl << "	}\n";
		}
		return tmpl.str();
	}

	string GetStaticMethodImpl(const Class& c)
	{
		map<string, vector<Function>> methods_by_name = GetMethodsByName(
				c.StaticMethods);

		stringstream tmpl;
		bool first = true;
		for (const auto& methods : methods_by_name)
		{
			tmpl << "	";
			if (first)
			{
				first = false;
			}
			else
			{
				tmpl << "else ";
			}
			tmpl << "if (name == \"" << methods.first << "\")\n";
			tmpl << "	{\n";
			for (const auto& method : methods.second)
			{
				string name = c.GetFullName() + "::" + methods.first;
				tmpl << "		results.push_back(std::make_unique<Function<"
					<< serializer::GetFunctionSignature(method) << ", " << name
					<< ">>());\n";
			}
			tmpl << "	}\n";
		}
		return tmpl.str();
	}

	string StaticMethodsDefinitions(const Class& c)
	{
		if (c.StaticMethods.empty())
		{
			return string();
		}

		stringstream tmpl;
		tmpl << "// " << c.GetFullName() << " static methods definitions.\n";

		for (const auto& method : c.StaticMethods)
		{
			serializer::SerializeFunctionSources(tmpl, method);
		}

		tmpl << "// End of " << c.GetFullName()
			<< " static methods definitions.\n";

		return tmpl.str();
	}

	string GetFieldImpl(
			const Class::FieldList& fields, const string& field_prefix)
	{
		stringstream tmpl;
		for (const auto& field : fields)
		{
			tmpl << "		if (name == \"" << field.Name << "\")\n";
			tmpl << "		{\n";
			tmpl << "			return Reference("
				<< field_prefix << field.Name << ");\n";
			tmpl << "		}\n";
		}
		return tmpl.str();
	}
}

void serializer::SerializeClassHeader(ostream& o, const Class& c,	const std::vector<std::unique_ptr<TypeBase>>& types)
{
	stringstream tmpl;
	tmpl << R"(
template <>
class Class<%name%> : public IClass
{
public:
	static const constexpr int FieldCount = %field_count%;
	static const constexpr int StaticFieldCount = %static_field_count%;
	static const constexpr int MethodCount = %method_count%;
	static const constexpr int StaticMethodCount = %static_method_count%;

	int GetFieldCount() const override;
	Reference GetField(
			const Reference& o, const std::string& name) const override;

	int GetStaticFieldCount() const override;
	Reference GetStaticField(const std::string& name) const override;

	int GetMethodCount() const override;
	std::vector<std::unique_ptr<IMethod>> GetMethod(
			const std::string& name) const override;

	int GetStaticMethodCount() const override;
	std::vector<std::unique_ptr<IFunction>> GetStaticMethod(
			const std::string& name) const override;

	const std::string& GetName() const override;

	// Calls T::operator() on each field of '%name%'.
	// Works well with C++14 generic lambdas.
	template <typename T>
	static void IterateFields(const %name%& c, T t);

	template <typename T>
	static void IterateFields(%name%& c, T t);

	template <typename T>
	static void IterateFieldsAndValues(const %name%& c, T t);

	template <typename T>
	static void IterateFieldsAndValues(%name%& c, T t);

	template <typename T>
	static void print_class_yaml(const %name% * c, std::string indent, T lprint);

	template <typename T>
	static void read_class_yaml( %name% * c, std::string indent, T lread);


	template <typename T>
	static void IterateStaticFields(T t);
};

template <typename T>
void Class<%name%>::IterateFields(const %name%& c, T t)
{
%iterate_fields%}

template <typename T>
void Class<%name%>::IterateFields(%name%& c, T t)
{
%iterate_fields%}

template <typename T>
void Class<%name%>::IterateFieldsAndValues(const %name%& c, T t)
{
%iterate_fields_and_values%}

template <typename T>
void Class<%name%>::IterateFieldsAndValues(%name%& c, T t)
{
%iterate_fields_and_values%}

template <typename T>
void Class<%name%>::print_class_yaml(const %name% * c, std::string indent, T lprint)
{
%print_class_yaml%
}

template <typename T>
void Class<%name%>::read_class_yaml( %name% * c, std::string indent, T lread)
{
%read_class_yaml%
}

template <typename T>
void Class<%name%>::IterateStaticFields(T t)
{
%iterate_static_fields%}



%methods_decl%%static_methods_decl%
)";

	o << ReplaceAll(
			tmpl.str(),
			{
				{"%name%", c.GetFullName()},
				{"%iterate_fields%", IterateFields(c)},
				{"%iterate_fields_and_values%", IterateFieldsAndValues(c, types, "    ", "c", false)},
				{"%print_class_yaml%", print_class_yaml(c, types, "", "c")},
				{"%read_class_yaml%", read_class_yaml(const_cast<Class&>(c), types, "", "c")},
				{"%iterate_static_fields%", IterateStaticFields(c)},
				{"%field_count%", to_string(c.Fields.size())},
				{"%static_field_count%", to_string(c.StaticFields.size())},
				{"%method_count%", to_string(c.Methods.size())},
				{"%methods_decl%", MethodsDeclarations(c)},
				{"%static_method_count%", to_string(c.StaticMethods.size())},
				{"%static_methods_decl%", StaticMethodsDeclarations(c)}
			});
}

void serializer::SerializeClassSources(ostream& o, const Class& c, const std::vector<std::unique_ptr<TypeBase>>& types)
{
	stringstream tmpl;
	tmpl << R"(
const int Class<%name%>::FieldCount;
const int Class<%name%>::StaticFieldCount;
const int Class<%name%>::MethodCount;
const int Class<%name%>::StaticMethodCount;

int Class<%name%>::GetFieldCount() const
{
	return FieldCount;
}

Reference Class<%name%>::GetField(const Reference& r, const std::string& name) const
{)";
	if (!c.Fields.empty())
	{
		tmpl << R"(
	if (r.IsT<%name%>())
	{
		%name%& o = r.GetT<%name%>();
%get_field_impl%	}
	else if (r.IsT<const %name%>())
	{
		const %name%& o = r.GetT<const %name%>();
%get_field_impl%	}
	else
	{
		throw Exception("Invalid Reference passed to GetField().");
	})";
	}
	tmpl << R"(
	throw Exception("Invalid name passed to GetField().");
}

int Class<%name%>::GetStaticFieldCount() const
{
	return StaticFieldCount;
}

Reference Class<%name%>::GetStaticField(const std::string& name) const
{
%get_static_field_impl%	throw Exception("Invalid name passed to GetStaticField().");
}

int Class<%name%>::GetMethodCount() const
{
	return MethodCount;
}

std::vector<std::unique_ptr<IMethod>> Class<%name%>::GetMethod(const std::string& name) const
{
	std::vector<std::unique_ptr<IMethod>> results;
%get_method_impl%
	return results;
}

int Class<%name%>::GetStaticMethodCount() const
{
	return StaticMethodCount;
}

std::vector<std::unique_ptr<IFunction>> Class<%name%>::GetStaticMethod(
		const std::string& name) const
{
	std::vector<std::unique_ptr<IFunction>> results;
%get_static_method_impl%
	return results;
}

static const std::string %escaped_name%_name = "%name%";

const std::string& Class<%name%>::GetName() const
{
	return %escaped_name%_name;
}

%method_definitions%%static_method_definitions%

namespace
{
	// Object to auto-register %name%.
	struct %escaped_name%_registrar
	{
		%escaped_name%_registrar()
		{
			::reflang::registry::internal::Register(
					std::make_unique<Class<%name%>>());
		}
	} %escaped_name%_instance;
})";

	o << ReplaceAll(
			tmpl.str(),
			{
				{"%name%", c.GetFullName()},
				{"%get_field_impl%", GetFieldImpl(c.Fields, "o.")},
				{
					"%get_static_field_impl%",
					GetFieldImpl(c.StaticFields, c.GetFullName() + "::")
				},
				{"%field_count%", to_string(c.Fields.size())},
				{"%escaped_name%", GetNameWithoutColons(c.GetFullName())},
				{"%method_definitions%", MethodsDefinitions(c)},
				{"%get_method_impl%", GetMethodImpl(c)},
				{"%static_method_definitions%", StaticMethodsDefinitions(c)},
				{"%get_static_method_impl%", GetStaticMethodImpl(c)}
			});
}
