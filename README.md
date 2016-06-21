# cookie-db
Cookie Database using PostgreSQL

The program takes an input file called cookie and loads it in a postgresql database which is created by the create.sql script

The UML description of the database structure is provided in the PDF file named database!

The program consists of 3 separate modules:
1. Create script which must be loaded into postgresql database - the script will create a schema called myproject

2. Make a call to make, to make with the command make all to make both load and query parts of the program
2.1 Run the load excutable in order to load all of the data into the database, file path is a full file path, or file name if the file is in the same folder as load and query, additional information is provided to standard in!

3. Query allows to query the database for a specific set of information which is hardcoded, you may change the query as you wish!
