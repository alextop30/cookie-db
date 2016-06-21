create schema myProject;

create type myProject.member_type as ENUM ('Daisy', 'Brownie', 'Junior', 'Senior', 'Cadette', 'Leader');

create table myProject.member(Name varchar(30), address varchar(50), type myProject.member_type, PRIMARY KEY(name, address));

create table myProject.Baker(Name varchar(30) PRIMARY KEY, Address varchar(50));

create table myProject.Cookie(Name varchar(30) PRIMARY KEY);

create table myProject.Council(Name varchar(30) PRIMARY KEY, Baker_name varchar(30) references myProject.baker(name));

create table myProject.Customer(name varchar(30), address varchar(50), PRIMARY KEY(name, address));

create table myProject.Date(Date date PRIMARY KEY);

create table myProject.service_unit(Number int, Name varchar(30), council_name varchar(30) references myProject.council(name), 
PRIMARY KEY (Number, council_name));

create table myProject.troop(Number int, serv_unit_num int, council_name varchar(30), 
PRIMARY KEY (number, serv_unit_num, council_name), 
foreign key(serv_unit_num, council_name) references myProject.service_unit(number, council_name));

create table myProject.sells_for(council_name varchar(30), cookie_name varchar(30), price money, 
PRIMARY KEY(council_name, cookie_name),
foreign key (council_name) references myProject.council(name),
foreign key (cookie_name) references myProject.cookie(name));

create table myProject.offers(baker_name varchar(30), cookie_name varchar(30), 
PRIMARY KEY(baker_name, cookie_name),
foreign key(baker_name) references myProject.baker(name),
foreign key(cookie_name) references myProject.cookie(name));

create table myProject.ind_sales(girl_name varchar(30), girl_address varchar(50), cust_name varchar(30), cust_address varchar(50), cookie_name varchar(30), date date, quantity int,
PRIMARY KEY (girl_name, girl_address, cust_name, cust_address, cookie_name, date),
foreign key(girl_name, girl_address) references myProject.member(name, address),
foreign key(cust_name, cust_address) references myProject.customer(name, address),
foreign key (cookie_name) references myProject.cookie(name),
foreign key (date) references myProject.date(date));

create table myProject.shop_sales(cookie_name varchar(30), date date, quantity int, troop_number int, troop_serv_unit_num int, troop_council_name varchar(30), 
PRIMARY KEY (cookie_name, date, troop_number, troop_serv_unit_num, troop_council_name),
foreign key (cookie_name) references myProject.cookie(name),
foreign key (date) references myProject.date(date),
foreign key (troop_number, troop_serv_unit_num, troop_council_name) references myProject.troop(number,serv_unit_num,council_name));

create table myProject.belongs_to(member_name varchar(30), member_address varchar(50), troop_number int, troop_serv_unit_num int, troop_council_name varchar(30),
PRIMARY KEY (member_name, member_address, troop_number, troop_serv_unit_num, troop_council_name),
foreign key (member_name, member_address) references myProject.member(name, address),
foreign key (troop_number, troop_serv_unit_num, troop_council_name) references myProject.troop(number,serv_unit_num,council_name));
