#include<iostream>
#include<string>
#include<postgresql/libpq-fe.h>
#include<stdexcept>
#include<stdlib.h>
#include<fstream>
#include<sstream>
#include<vector>
#include<utility>
#include<cctype>


using std::cout;
using std::endl;
using std::cerr;
using std::cin;
using std::vector;
using std::pair;
using std::ifstream;
using std::string;
using std::logic_error;
using std::istringstream;
using std::getline;

//connect function prototype
PGconn* connect(vector<string>&);
vector< string> parse_line( string line);
void print_vec(vector< string> parsed);
void print_table(vector< pair <string,string> > vals);
vector< string> get_table_names();
vector< string> get_pk_att(string table);
vector< pair <string,string> > get_vals( string table,  vector<string> &parsed_line);
vector<string> get_column_names(string table);
void insert(vector< string> parsed);
string assemble_query(vector< pair <string,string>> data, vector<string> pk_att, string table);
string assemble_insert_query(vector< pair <string,string>> data, string table);
bool is_present(string query);
bool is_only_digit(string value);

//database handler (gloval var)
PGconn *mydb = NULL;

//all table names
vector<string> table_names;

//global debug flag
int debug = 0;

//global schema name
string schema;

int main (void)
{
	string file_name;
	ifstream input_file;
	string data;
	string db;
    string temp;

	vector< string> parsed;
	vector< string> tNames;
	vector< string> pk;

	//connection information vector
    vector<string> conn;

	//Did the user select debug?
	cout << "Do you want to turn on debug? Y/N:  ";
	cin >> db;
	
	//check for the debug flag
	if(db == "Y" || db =="y")
		debug = 1;
	else
		debug = 0;
	
	//let user know debugging is on	
	debug && cout << "Debugging is on!" << endl;

	//file path
	cout << "Specify input file name that is in the current directory or complete file path: ";
	cin >> file_name;

	cout << "Enter host name of the database: ";
	cin>>temp;

	//get host into the connection vector
	conn.push_back(temp);

	//dbname
    cout << "Enter database user name: ";
    cin >> temp;
    
    //put in conn vector
    conn.push_back(temp);
    
    //get password
    cout << "Enter password: ";
    cin >> temp;
    conn.push_back(temp);
    
    //get dbname
    cout << "Enter database name: ";
    cin>> temp;

	//save the dbname
    conn.push_back(temp);

	//get the schema name
	cout << "Enter schema name: ";
	cin>>schema;
	
	//open input file stream to the file
	input_file.open(file_name);
	
	//check the stream
	if(input_file.good())
	{
		 cout << "The input file was opened successfully!" <<  endl;
	}
	else
	{
		 cout << "Input file was not opened, terminating!" <<  endl;
		exit(EXIT_FAILURE);
	}
	
	try
	{
		//connect to the database
		mydb = connect(conn);
		
		//get table names and initialize global var
		table_names = get_table_names();
	}
	catch(...)
	{
		//report another error message
		 cerr << "Bad connection to database!" <<  endl;
		
		//exit with fail
		exit(EXIT_FAILURE);
	}
	
	//print_vec(table_names);
	
	//get a line from the file
	 getline(input_file, data);
	
	while(!input_file.eof())
	{
		
		//save the vector in parsed
		parsed = parse_line(data);
		
		//call to insert function
		insert(parsed);	
		
		//get a line from the file
		 getline(input_file, data);
	}
	
	//let user know we closed the connection
	 cout<< "Closing database connection" <<  endl;
	
	//close the connection
	PQfinish(mydb);

	return 0;
}

/*Function connects to the database, if connection fails
 * function will report a readable error to the console
 * - function returns a point to a PGconn object for further
 * access to the database.
 */ 
PGconn* connect(vector<string>& conn)
{

	//construct the connection string using user input
	string connection = "host=" + conn[0] + " user=" + conn[1] + " password=" + conn[2] + " dbname=" + conn[3];

	debug && cout << "Connection string: " + connection << endl;

	//establish connection with the db
	PGconn *mydb = PQconnectdb(connection.c_str());

	//check for successful connection, if conection is not established report error
	if(PQstatus(mydb) == CONNECTION_BAD)
    {
         cerr << "Connection to database failed with: "
        << PQerrorMessage(mydb) <<  endl;

		throw( logic_error("Connection to database unsuccessful"));
    }
    else
    {
		//if there was successful connection report it to the console
         cout << "Connection to database is successful!" << endl << endl;
    }

	//return the conection handler to be used above
	return mydb;
	
}

/* Function goes through the 1 line passed in from the file
 * and splits it according to the pipe character which is
 * the defult delimiter. Inside the line and the \n is
 * the line delimiter which is why double parsing is needed
 */
 vector< string> parse_line(string line)
{
	//string stream to deal with the parsing
	 istringstream ss(line);
	
	//token to hold the current word
	 string token;
	
	//vector to hold all of the parsed tokens
	 vector< string> tokenized_string;
	
	//go through the line and tokenize it and put it in the vector
	while( getline(ss, token, '|'))
	{
		tokenized_string.push_back(token);
	}
	
	//return tokenized_vector
	return tokenized_string;
	
}


//utility function prints the tokens of the parsed line
//function is used for debugging purposes only
void print_vec(vector< string> parsed)
{
	//go through vector and print it on the console
	for(size_t i = 0; i < parsed.size(); i++)
	{
		 cout<< parsed[i] << " ";
	}
	
	 cout << "\n";
}


/*funciton makes a query to the database and obtains
 * the database names within the schema that is student id
 * it is hardcoded into the function
 */
vector<string> get_table_names()
{
	//will save table names and will return them out of the function
	 vector< string> table_names;

	string query = "select tablename from pg_tables where schemaname='" + schema + "' order by tablename asc";

	//execute query to get table names out of the metadata of db
	PGresult * res = PQexec(mydb, query.c_str());
	
	debug && cout << "Getting table names from db!" << endl;
	
	//if something goes wrong or schema is not there print error and exit
	if(PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		cerr << "Schema does not exist, launch create.sql first to create tables" << endl;
		PQclear(res);
		PQfinish(mydb);
		exit(EXIT_FAILURE);
	}
	
	//get the number of rows
	int rows = PQntuples(res);
	
	//if we get an empty result signal error and exit
	if(rows == 0)
	{
		cerr << "Database is lacking tables run create.sql to" 
		" create the proper tables!" << endl;
		PQclear(res);
		PQfinish(mydb);
		exit(EXIT_FAILURE);		
	}
	
	//get table names
	for(int i = 0; i< rows; i++)
	{
		table_names.push_back(PQgetvalue(res,i,0));
	}
	
	
	PQclear(res);
	
	//return table list
	return table_names;
}

/* Function makes a call to the database getting the table
*  passed into the function's primary keys so that they 
*  can be used in subsequent functions and be inserted
*  values for. If primary keys are not obtained function will
*  throw an error and exit the program.
*/

 vector< string> get_pk_att(string table)
{
	//will save table names and will return them out of the function
	 vector< string> pk_att;
	
	//construct the query
	 string query = "SELECT a.attname, format_type(a.atttypid, a.atttypmod) " 
	"AS data_type FROM pg_index i JOIN pg_attribute a ON a.attrelid = i.indrelid"
	" AND a.attnum = ANY(i.indkey) WHERE i.indrelid = '" +schema + "."+ table +
	"'::regclass AND i.indisprimary order by attname";
	
	debug && cout << "Getting primary key attributes for table:  " << table << endl;
	
	//execute query to get table names out of the metadata of db
	PGresult * res = PQexec(mydb, query.c_str());
	
	//if something goes wrong or schema is not there print error and exit
	if(PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		cerr << "Cannot get table primary key attribute names!" << endl;
		PQclear(res);
		PQfinish(mydb);
		exit(EXIT_FAILURE);
	}
	
	//get the number of rows
	int rows = PQntuples(res);
	
	//if there is no result for primary keys for the table
	//print that table is not available and exit
	if(rows == 0)
	{
		cerr << table << " does not exist, run create.sql script to" 
		" create the proper tables" << endl;
		PQclear(res);
		PQfinish(mydb);
		exit(EXIT_FAILURE);		
	}
	
	//get table names
	for(int i = 0; i< rows; i++)
	{
		pk_att.push_back(PQgetvalue(res,i,0));
	}
	
	//release the handle of result
	PQclear(res);
	
	//return table list
	return pk_att;
	
}



/*
* Function get values gets the hardcoded values for each one of the tables
* from the line that was parsed by the previous functions. The data
* is hardcoded because of the lack of headers in the file providing
* needed infromation of what the values are. Return is an array of
* pairs where the key is the attribute name, the value is the value
* that is from the file
*/

vector<pair< string,string>> get_vals( string table,  vector<string> &parsed_line)
{
	 //vector of pairs that 1st pair holds attribute name
	 //second pair holds attribute value
	 vector<pair< string,string>> vals;
	 
	 //attributes from database
	 vector<string> col_names;
	 
	if(table == table_names[0])
	{		
		//baker table
		debug && cout << "Getting values for table: Baker!" << endl;
		
		col_names = get_column_names(table_names[0]);
		vals.push_back(make_pair(col_names[0], parsed_line[14]));
		vals.push_back(make_pair(col_names[1], parsed_line[13]));
	}
	else if(table == table_names[1])
	{
		//belongs to table
		
		debug && cout << "Getting values for table: Belongs_to!" << endl;
		
		col_names = get_column_names(table_names[1]);
		
		//leader name and address
		vals.push_back(make_pair(col_names[0], parsed_line[5]));
		vals.push_back(make_pair(col_names[1], parsed_line[4]));
		
		//girl name and address
		vals.push_back(make_pair(col_names[0], parsed_line[7]));
		vals.push_back(make_pair(col_names[1], parsed_line[6]));
		
		//rest of the attributes
		vals.push_back(make_pair(col_names[2], parsed_line[0]));
		vals.push_back(make_pair(col_names[3], parsed_line[3]));
		vals.push_back(make_pair(col_names[4], parsed_line[2]));
	}
	else if(table == table_names[2])
	{
		//table cookie
		
		debug && cout << "Getting values for table: Cookie!" << endl;
		
		col_names = get_column_names(table_names[2]);
		vals.push_back(make_pair(col_names[0], parsed_line[11]));
	}
	else if(table == table_names[3])
	{
		//table council
		
		debug && cout << "Getting values for table: Council!" << endl;
		
		col_names = get_column_names(table_names[3]);
		
		//get the values into the vector
		vals.push_back(make_pair(col_names[0], parsed_line[13]));
		vals.push_back(make_pair(col_names[1], parsed_line[0]));
		

	}
	else if(table == table_names[4])
	{
		//customer table
		
		debug && cout << "Getting values for table: Customer!" << endl;
		col_names = get_column_names(table_names[4]);
		
		//get the values
		vals.push_back(make_pair(col_names[0], parsed_line[10]));
		vals.push_back(make_pair(col_names[1], parsed_line[9]));
	}
	else if(table == table_names[5])
	{
		//date table
		
		debug && cout << "Getting values for table: Date!" << endl;
		
		col_names = get_column_names(table_names[5]);
		
		//get values
		vals.push_back(make_pair(col_names[0], parsed_line[16]));

	}
	else if(table == table_names[6])
	{
		//individual sales table
		
		debug && cout << "Getting values for table: Ind_sales!" << endl;
		
		col_names = get_column_names(table_names[6]);
		
		vals.push_back(make_pair(col_names[0], parsed_line[11]));
		vals.push_back(make_pair(col_names[1], parsed_line[10]));
		vals.push_back(make_pair(col_names[2], parsed_line[9]));
		vals.push_back(make_pair(col_names[3], parsed_line[16]));
		vals.push_back(make_pair(col_names[4], parsed_line[7]));
		vals.push_back(make_pair(col_names[5], parsed_line[6]));
		vals.push_back(make_pair(col_names[6], parsed_line[15]));
	}
	else if(table == table_names[7])
	{
		//member
		
		debug && cout << "Getting values for table: Member!" << endl;
		
		col_names = get_column_names(table_names[7]);
		
		//get values
		
		//leader name and address
		vals.push_back(make_pair(col_names[1], parsed_line[4]));
		vals.push_back(make_pair(col_names[0], parsed_line[5]));
		
		//girl name and address
		vals.push_back(make_pair(col_names[1], parsed_line[6]));
		vals.push_back(make_pair(col_names[0], parsed_line[7]));
		
		//leader/girl flag - decided later
		vals.push_back(make_pair(col_names[2], ""));
		
	}
	else if(table == table_names[8])
	{
		//offers table
		
		debug && cout << "Getting values for table: Offers!" << endl;
		
		col_names = get_column_names(table_names[8]);
		
		vals.push_back(make_pair(col_names[0], parsed_line[13]));
		vals.push_back(make_pair(col_names[1], parsed_line[11]));
	}
	else if(table == table_names[9])
	{
		//sells_for table
		
		debug && cout << "Getting values for table: Sells_for!" << endl;
		col_names = get_column_names(table_names[9]);
		
		vals.push_back(make_pair(col_names[0], parsed_line[11]));
		vals.push_back(make_pair(col_names[1], parsed_line[0]));
		vals.push_back(make_pair(col_names[2], parsed_line[12]));
	}
	else if(table == table_names[10])
	{
		//service unit table
		
		debug && cout << "Getting values for table: Service_unit!" << endl;
		col_names = get_column_names(table_names[10]);
		
		vals.push_back(make_pair(col_names[0], parsed_line[0]));
		vals.push_back(make_pair(col_names[1], parsed_line[1]));
		vals.push_back(make_pair(col_names[2], parsed_line[2]));

	}
	else if(table == table_names[11])
	{
		//shop sales table
		debug && cout << "Getting values for table: Shop_sales!" << endl;
		col_names = get_column_names(table_names[11]);
		
		//get values
		vals.push_back(make_pair(col_names[0], parsed_line[11]));
		vals.push_back(make_pair(col_names[1], parsed_line[16]));
		vals.push_back(make_pair(col_names[2], parsed_line[15]));
		vals.push_back(make_pair(col_names[3], parsed_line[0]));
		vals.push_back(make_pair(col_names[4], parsed_line[3]));
		vals.push_back(make_pair(col_names[5], parsed_line[2]));
	}
	else if(table == table_names[12])
	{
		//troop table
		debug && cout << "Getting values for table: Troop!" << endl;
		
		col_names = get_column_names(table_names[12]);
		
		//get values
		vals.push_back(make_pair(col_names[0], parsed_line[0]));
		vals.push_back(make_pair(col_names[1], parsed_line[3]));
		vals.push_back(make_pair(col_names[2], parsed_line[2]));
	}

	
	return vals;
	
	
}

/*
* Get column names makes a call to do the database and obtains all of the
* column names for the particular table that is passed into the function
* if an error is returned from the query error is reported and program
* exits.
*/
vector<string> get_column_names(string table)
{
	//vector to hold attribute names for particular table
	vector<string> col_names;
	
	//construct the query for getting attribute names from the schema of db
	string query = "SELECT column_name FROM information_schema.columns WHERE"
	" table_schema = '" + schema + "' AND table_name   = '" + table + "' order by column_name asc";
	
	debug && cout << "Getting all attributes for table: " << table << endl;
	
	//execute query to get table names out of the metadata of db
	PGresult * res = PQexec(mydb, query.c_str());
	
	//if something goes wrong or schema is not there print error and exit
	if(PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		cerr << "Could not get table columns!" << endl;
		PQclear(res);
		PQfinish(mydb);
		exit(EXIT_FAILURE);
	}
	
	//get the number of rows
	int rows = PQntuples(res);
	
	//get col names
	for(int i = 0; i< rows; i++)
	{
		col_names.push_back(PQgetvalue(res,i,0));
	}
	
	//release the handle of result
	PQclear(res);
	
	//return the vector of attribute names
	return col_names;
}

//utility function to print the values in the vector of pairs
//used for debug only
void print_table(vector< pair <string,string> > vals)
{
	for (size_t i = 0; i < vals.size(); i++)
	{
		cout << vals[i].first << "  ";
		cout << vals[i].second << "  ";
	}
	
	cout << endl;
	
}

//assembles a select query for each table passed in
//to make sure that the data is not in the table
//this query does not insert into any tables
string assemble_query(vector< pair <string,string>> data, vector<string> pk_att, string table)
{
	string query = "Select ";
	int er;
	
	debug && cout << "Adding Primary Key Attributes to query!" << endl;
	
	//add the primary key attribbutes to the query
	for (size_t i = 0; i < pk_att.size(); i++)
	{
		//add attributes
		query = query + pk_att[i];
		
		//make sure there are enough commas
		if(i != pk_att.size()-1)
			query = query + ", ";		
	}
	
	//add the schema and table and where clause
	query = query + " from " +  schema + "." + table + " where ";
	
	//nested loops that go through the primary keys and match them with
	//the data attributes. If they match the primary keys are added to the
	//select query. All data is escaped using the appropriate postggres function
	for(size_t i = 0; i < pk_att.size(); i++)
	{
		for(size_t j=0; j < data.size(); j++)
		{
			if(pk_att[i] == data[j].first)
			{
				
				//check for empty primary keys, if empty get of the function
				if(data[j].second.length() == 0)
				{
					cout << "Empty primary key, continuing to next table" << endl;
					return "empty";
				}				
				
				//save string while also escaping all characters that may exist
				query = query + data[j].first + " = ";
				
				if(is_only_digit(data[j].second))
				{
					//if we have an integer we should not escape it
					query = query + data[j].second;
					debug && cout << "PK data is a number adding to query!" << endl;
				}
				else
				{
					debug && cout << "PK data is not a number escaping!" << endl;
					//make sure that everythign is escaped properly in the data
					//so that the query does run properly
					int size = (data[j].second.length()*2)+1;
					char buf[size];
					PQescapeStringConn(mydb ,buf, data[j].second.c_str(), data[j].second.length(), &er);
					string temp = buf;
					temp.shrink_to_fit();
					query = query + "'" + temp + "'";
					
					//if errors are encountered at the time of escaping
					//shut down the program, could face injection problems
					if(er != 0)
					{
						cerr << "Error in escaping data from file, to "
						"preserve db integrity exiting!" << endl;
						PQfinish(mydb);
						exit(EXIT_FAILURE);
					}
				}
				
				//add the correct number of AND to the query
				if(i != pk_att.size()-1)
					query = query + " AND ";
			}
		}
		
	}
	
	//print out table and entire query if debug is on
	debug && cout << endl << endl <<"Select query for Table: " << table << endl;
	debug && cout << "Assembled Select Query" << query << endl;
	
	//return the formatted query
	return query;
}


/*
* Insert function is the one that makes all of the decisions of
* how and when information should be inserted. It makes calls
* to all other utility functions allowing the decision making
* weird logic is used for girl and leader in the member and 
* belongs to tables because all of the information is saved
* in the same vector of pairs
*/
void insert(vector< string> parsed)
{	
	//strings to hold query and table name
	string query;
	string table;
	
	//reassign table names in the order they need to be processed
	//tables need to be in this order due to foreign key constraints
	vector<string> t_names = {"baker", "cookie", "customer", 
		"council", "date", "member", "service_unit", "troop", 
		"belongs_to", "offers", "sells_for", "shop_sales", "ind_sales"};
	
	//set up a result handler
	PGresult * res;
	
	//go through all tables performing insert functions
	for(size_t i = 0; i < t_names.size(); i++)
	{
		table = t_names[i];
		
		debug && cout << "Table being examined for queries, Insert Function!" << endl;
		debug && cout << "Table: " << table << endl;
		
		cout << "Examining Table: " << table << endl;
		
			vector<string> pk_att = get_pk_att(table);
			vector< pair <string,string>> data = get_vals(table, parsed); 
		
		if(table == "member" || table == "belongs_to")
		{
			//girl information is saved at these locations
			//remove it and store it in tep locations so that
			//2 tests can be performed
			pair<string, string> p1 (data[2].first, data[2].second);
			pair<string, string> p2 (data[3].first, data[3].second);
			
			//delete the girl data
			data.erase(data.begin()+2, data.begin()+4);
			
			//assemble the query for leader
			query = assemble_query(data, pk_att, table);
			
			//if one of the pks was empty check girl
			if(query == "empty")
				goto check_girl;
			
			if(!is_present(query))
			{
				debug && cout << "Previous select query not in db, inserting!" << endl;
				
				cout << "Leader information was not present in the Database, it was inserted" << endl;
				
				if(debug)
				{
					debug && cout << "Data from parsed line which will be inserted!" << endl;
					print_table(data);
				}
				
				//set leader flag in member table
				if(table == "member")
					data[2].second = "Leader";
				
				//assembble the insert query
				query = assemble_insert_query(data, table);
				
				//if empty fields are found information will not be inserted
				//error is signled go to the girl check
				if(query == "empty")
					goto check_girl;
				
				debug && cout << "Leader insert query! "<< endl << query << endl;
				
				//execute the query
				res = PQexec(mydb, query.c_str());
				
				//check if insert command was successful
				if(PQresultStatus(res) != PGRES_COMMAND_OK)
				{
					cerr<< "Error in Insert Query: " 
						<< PQresultErrorMessage(res) << endl;
				}
				
			
			}
			else
				cout << "Leader information was present in Database, skipping!" << endl;
			
			//label to skip if pk is empty
			check_girl:
			
			//reassign the girl stats to make sure
			//we check with girl
			data[0] = p1;
			data[1] = p2;
			
			debug && cout << "Check for girl, if she exists in table!" << endl;
			
			//assemble query for girl
			query = assemble_query(data, pk_att, table);
			
			//if girl information comes back empty, 
			//go to the next iteration of the loop
			if(query == "empty")
				continue;
			
			//if not present insert into the table
			if(!is_present(query))
			{
				//if the girl information is not present insert it
				
				debug && cout << "Girl select query not in db, inserting!" << endl;
				
				cout << "Girl information was not present in the Database, it was inserted" << endl;
				
				//set the rank
				if(table == "member")
					data[2].second = parsed[8];
				
				//asseble the insert query
				query = assemble_insert_query(data, table);
				
				//if empty field is found continue to next table
				//error was already reported
				if(query == "empty")
					continue;
				
				debug && cout << "Girl insert query!" << endl << query << endl;
				
				//execute the query
				res = PQexec(mydb, query.c_str());
				
				//check if insert command was successful
				if(PQresultStatus(res) != PGRES_COMMAND_OK)
				{
					cerr<< "Error in Insert Query: " 
						<< PQresultErrorMessage(res) << endl;
				}
			}
			else
				cout << "Girl information was present in Database, skipping!" << endl;

		}
		else
		{
			//assemble select query
			
			debug && cout << "Not in special case tables! Assembling select query" << endl;
			
			query = assemble_query(data, pk_att, table);
			
			//if we have empty query for pks 
			//continue to the next iteration of the loop
			if(query == "empty")
				continue;
			
			//if select query does not yield results insert into table
			if(!is_present(query))
			{
				debug && cout << "Previous query was not found in db, Inserting!" << endl;
				
				cout << "Information was not present in the Database, it was inserted" << endl;
				
				//asseble insert query
				query = assemble_insert_query(data, table);
				
				//if empty field is found continue to next table
				//error was already reported
				if(query == "empty")
					continue;
				
				debug && cout << "Insert query for non special tables!" << endl << query << endl;
				
				res = PQexec(mydb, query.c_str());
				
				//check if insert command was successful
				if(PQresultStatus(res) != PGRES_COMMAND_OK)
				{
					cerr<< "Error in Insert Query: " 
						<< PQresultErrorMessage(res) << endl;
				}
			}
			else
				cout << "Information was present in Database, skipping!" << endl;
				
		}

	}
}

//is present makes a call to the database executing the select
//query assembled in the function above. If there is a result that
//has more than 0 column, the information is in the table and it
//returns false, if information is present the function returns true
bool is_present(string query)
{
	//get a result handler and execute the query passed in
	PGresult *res = PQexec(mydb, query.c_str());
	
	//if tuples are = 0  meaning empty set info is not present	
	if (PQntuples(res) == 0)
	{
		return false;
	}
	
	//if there is more than 0 tuples information is present
	return true;
}

//is digit checks a string for all of the characters being digits
//this is used to check for values that should not be escaped
bool is_only_digit(string value)
{
	for (size_t i =0; i < value.length(); i++)
	{
		if(!isdigit(value[i]))
			return false;
			
	}
	
	return true;
}

//assembles insert query appropriately and it escapes all data
//using the proper postgres function	
string assemble_insert_query(vector< pair <string,string>> data, string table)
{
	debug && cout << "Assembling insert query!" << endl;
	
	//start with key words schema and table name
	string query = "INSERT INTO " + schema + "." +table + " (";
	
	//to hold error flags
	int er;

	//go through each one of the attribute names and put them into
	//the query.
	for(size_t j = 0; j < data.size(); j++)
	{	
		if(data[j].second.length() == 0)
		{
			cout << "Empty attribute values found continuing to next table!"
			<< endl;
			return "empty";
		}
			
		query = query + data[j].first;
		
		if(j != data.size() -1)
			query = query + ", ";
	}
	
	//add additional information for formatting
	query = query + ") VALUES (";
	
	//go through each one of the values escape them and check for
	//only numbers in the values -- numbers are not escaped
	for(size_t j = 0; j < data.size(); j++)
	{	
		//make sure that everything is escaped properly in the data
		//so that the query does run properly
		int size = (data[j].second.length()*2)+1;
		char buf[size];
		PQescapeStringConn(mydb ,buf, data[j].second.c_str(), data[j].second.length(), &er);
		string temp = buf;
		temp.shrink_to_fit();
		
		if(is_only_digit(data[j].second))
		{
			debug && cout << "Value is a number, NOT escaped!" << endl;
			
			//if we have an integer we should not escape it
			query = query + data[j].second;
		}
		else
			query = query + "'"+ temp + "'";
		
		if(j != data.size() -1)
			query = query + ", ";
	}
	
	query = query + ")";
	
	//return completed query!
	return query;
}
