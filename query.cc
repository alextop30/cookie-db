#include<iostream>
#include<string>
#include<stdexcept>
#include<postgresql/libpq-fe.h>
#include<stdlib.h>
#include<vector>
#include<climits>
#include<iomanip>

using std::cout;
using std::cin;
using std::logic_error;
using std::cerr;
using std::string;
using std::endl;
using std::vector;
using std::to_string;
using std::stoi;
using std::stof;
using std::setw;
using std::right;
using std::left;
using std::fixed;
using std::setprecision;

//function prototypesa
PGconn* connect(vector<string>&);
string clean_input(string council_name);
void query_db(string council_name, int troop_num);

//database handler (global var)
PGconn *mydb = NULL;
//global debug flag
int debug = 0;

//global var for schema that needs
//to be inputted by user
string schema;

int main(void)
{
	string council_name;
	int troop_num;
	string db;
	string temp;
	
	cout << "Do you want to turn on debug? Y/N " ;
	cin >> db;
	
	//check for the debug flag
	if(db == "Y" || db =="y")
		debug = 1;
	else
		debug = 0;

	//get necessary information for login

	//connection information vector
	vector<string> conn;

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

	//connect to the db
	try
	{
		//connect to the database
		mydb = connect(conn);
	}
	catch(...)
	{
		//report another error message
		 cerr << "Bad connection to database!" <<  endl;
		
		//exit with fail
		exit(EXIT_FAILURE);
	}
	
	
	//get the information from the user
	cout << "Enter council name: " ;
	
	//flush the cin buffer
	cin.ignore(INT_MAX, '\n');
	
	//get a line from cin
	getline(cin, council_name);
	
	debug && cout << "Council Name Selected: " << council_name << endl;
	
	cout << "Enter troop number: ";
	cin >> troop_num; 
	
	debug && cout << "Troop Number Selected: " << troop_num << endl;

	try
	{
		//get clean input
		council_name = clean_input(council_name);
		
		debug && cout << "The cleaned council name is: " << council_name << endl;
		
		query_db(council_name, troop_num);
	}
	catch(...)
	{
		PQfinish(mydb);
		exit(EXIT_FAILURE);
	}

	//close the connection
	PQfinish(mydb);

	return 0;
}

//connect function connects to the database using hardcoded information
//it throws an error if the connection is not established properly
PGconn* connect(vector<string>& conn)
{
	//construct the connection string using user input
	string connection = "host=" + conn[0] + " user=" + conn[1] + " password=" + conn[2] + " dbname=" + conn[3];

	debug && cout << "Connection string: " + connection << endl;

	//establish connection with the db
	PGconn *mydb = PQconnectdb(connection.c_str());

	//check for successful connection, if connection is not established report error
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

	//return the connection handler to be used above
	return mydb;

}

//cleans the council name and espcapes it using the proper
//postgresql function. The function does make a call to the
//database getting the maximum council name length and comparing
//it to the length of the inputted string. If the inputted sting
//length is larger the function reports error.
string clean_input(string council_name)
{
	string query = "select max(length(name)) from " + schema + ".council";

	debug && cout << "Query for max length of name: " << query << endl;

	//execute query to get table names out of the metadata of db
	PGresult * res = PQexec(mydb, query.c_str());
	
	if(PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		cerr<<"Error getting the maximum council name from the database!" << endl;
		cerr << "Exiting!" << endl;
		PQfinish(mydb);
		exit(EXIT_FAILURE);
	}
	
	//get the max number of characters the council name can be
	int max_char = atoi(PQgetvalue(res,0,0));
	
	debug && cout << "Maximum number of characters allowed for council: " << max_char << endl;
	
	//if the string is past the maximum number of characters end the function with error Not found
	if((int) council_name.length() > max_char)
	{
		debug && cout << "No council name found! Length of sting is too great" << endl;
		
		throw(logic_error("No council name found!"));
	}
	
	//size of buf is 2 times size of input str + 1 	
	int size = (council_name.length() *2) + 1;
	
	//set up destination for escaped string
	char buf[size];
	
	//errors generated in escaping
	int er;
	
	//escape the string
	PQescapeStringConn(mydb ,buf, council_name.c_str(), council_name.length(), &er);
	
	//if error occured terminate the function
	if(er != 0)
		throw(logic_error("Error in escaping Council Name entered!"));
	
	
	//get the new string
	council_name = buf;
	
	//shrink it to size
	council_name.shrink_to_fit();
	
	//return the new escaped string
	return council_name;
}


//Assembles the query with the provided escaped infomration
//and prints the output in a table format
//the function will report empty result from the database
//on error the function exists the entire program
void query_db(string council_name, int troop_num)
{
	//assemble the query
	string query = "select sum(quantity),shop_sales.cookie_name,price from " +
	schema + ".shop_sales, "+ schema + ".sells_for where sells_for.cookie_name "
	"= shop_sales.cookie_name and shop_sales.troop_council_name = "
	"sells_for.council_name and troop_number = " + to_string(troop_num) + 
	" and council_name = '"+ council_name + 
	"' group by shop_sales.cookie_name,price";
	
	debug && cout << "Query being executed: " << endl << endl 
	<< query << endl;
	
	//execute query to get table names out of the metadata of db
	PGresult * res = PQexec(mydb, query.c_str());
	
	if(PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		cerr<<"Error contacting the database!" << endl;
		cerr << "Exiting!" << endl;
		PQfinish(mydb);
		exit(EXIT_FAILURE);
	}
	
	//get the number of rows
	int rows = PQntuples(res);
	int col = PQnfields(res);
	
	debug && cout << "Number of tuples: " << rows << endl;
	debug && cout << "Number of columns: " << col << endl;
	
	//check if we have an empty result from the db
	if(rows == 0)
	{
		cout << "Invalid council or troop entered!" << endl;
		
		return;
		
	}
	
	cout << endl << endl << endl;
	
	
	cout << "-----------------------------------------------------------" << endl;
	cout << setw(15) << "Number of Boxes"  << setw(18) << "Cookie Type" 
	<< setw(26) << "Total Amount" <<endl;
	
	cout << "-----------------------------------------------------------" << endl;
	
	//declarations used in loop
	int qty;
	string temp;
	float total;
	
	//turn everything into a float by deving by this value
	float constant = 1.0;
	
	//string size type used for stof
	std::string::size_type sz; 
	
	cout << fixed <<setprecision(2);
	
	//go through the result and grab the information
	//go through rows
	for(int i = 0; i < rows; i++)
	{
		//get the number of boxes
		qty = stoi(PQgetvalue(res,i,0));
		
		debug && cout << "Quantity: " << qty << endl;
		
		//get the get the price per box
		temp = PQgetvalue(res,i,2);
		
		debug && cout << "Value of temp: " << temp << endl;
		
		//get rid of the dollar sign
		temp = temp.substr(1, temp.length()-1);
		
		//get the total for the type of cookies
		total = qty * (stof(temp, &sz) / constant) ;
		
		debug && cout << "Total: " << total << endl;
		
		
		//print out the final result table
		
		cout <<"    "<< left << setw(15) <<PQgetvalue(res,i,0);
		
		
		cout << setw(25) <<PQgetvalue(res,i,1);
		cout << right << setw(6) << "$" <<total;	
		cout << endl;
		cout << "-----------------------------------------------------------" << endl;
	}
	
}
	
	

