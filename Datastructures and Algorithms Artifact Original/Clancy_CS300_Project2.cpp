//============================================================================
// Name        : Clancy_CS300_Project2.cpp
// Author      : Joe Clancy
// Course      : CS300
// Date        : 8/10/2024
// Description : 7-1 Project Two
//============================================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>
#include <list>
#include <algorithm>

using namespace std;

//============================================================================
// Global definitions visible to all methods and classes
//============================================================================

// DataType
enum DataType {
    eFILE = 0,
    ePURE = 1
};

// forward declarations
double strToDouble(string str, char ch);

// define a structure to hold course information
struct Course {
    string courseId; // unique identifier
    string courseName;
    vector<string> preReqList;
};

// Internal structure for tree node
struct Node {
    Course course;
    Node* left;
    Node* right;

    // default constructor
    Node() {
        left = nullptr;
        right = nullptr;
    }

    // initialize with a course
    Node(Course aCourse) :
        Node() {
        course = aCourse;
    }
};

//============================================================================
// Error class definition
//============================================================================

class Error : public std::runtime_error
{

public:
    Error(const std::string& msg) :
        std::runtime_error(std::string("CSVparser : ").append(msg))
    {
    }
};

//============================================================================
// Row class definition
//============================================================================

class Row
{
public:
    Row(const std::vector<std::string>&);
    ~Row(void);

public:
    unsigned int size(void) const;
    void push(const std::string&);
    bool set(const std::string&, const std::string&);

private:
    const std::vector<std::string> _header;
    std::vector<std::string> _values;

public:

    template<typename T>
    const T getValue(unsigned int pos) const
    {
        if (pos < _values.size())
        {
            T res;
            std::stringstream ss;
            ss << _values[pos];
            ss >> res;
            return res;
        }
        throw Error("can't return this value (doesn't exist)");
    }
    const std::string operator[](unsigned int) const;
    const std::string operator[](const std::string& valueName) const;
    friend std::ostream& operator<<(std::ostream& os, const Row& row);
    friend std::ofstream& operator<<(std::ofstream& os, const Row& row);
};


//============================================================================
// Row member functions
//============================================================================

Row::Row(const std::vector<std::string>& header)
    : _header(header) {}

Row::~Row(void) {}

unsigned int Row::size(void) const
{
    return _values.size();
}

void Row::push(const std::string& value)
{
    _values.push_back(value);
}

bool Row::set(const std::string& key, const std::string& value)
{
    std::vector<std::string>::const_iterator it;
    int pos = 0;

    for (it = _header.begin(); it != _header.end(); it++)
    {
        if (key == *it)
        {
            _values[pos] = value;
            return true;
        }
        pos++;
    }
    return false;
}

const std::string Row::operator[](unsigned int valuePosition) const
{
    if (valuePosition < _values.size())
        return _values[valuePosition];
    throw Error("can't return this value (doesn't exist)");
}

const std::string Row::operator[](const std::string& key) const
{
    std::vector<std::string>::const_iterator it;
    int pos = 0;

    for (it = _header.begin(); it != _header.end(); it++)
    {
        if (key == *it)
            return _values[pos];
        pos++;
    }

    throw Error("can't return this value (doesn't exist)");
}

std::ostream& operator<<(std::ostream& os, const Row& row)
{
    for (unsigned int i = 0; i != row._values.size(); i++)
        os << row._values[i] << " | ";

    return os;
}

std::ofstream& operator<<(std::ofstream& os, const Row& row)
{
    for (unsigned int i = 0; i != row._values.size(); i++)
    {
        os << row._values[i];
        if (i < row._values.size() - 1)
            os << ",";
    }
    return os;
}

//============================================================================
// Parser class definition
//============================================================================

class Parser
{

public:
    Parser(const std::string&, const DataType& type = eFILE, char sep = ',', bool head=false);
    ~Parser(void);

public:
    Row& getRow(unsigned int row) const;
    unsigned int rowCount(void) const;
    unsigned int columnCount(void) const;
    std::vector<std::string> getHeader(void) const;
    const std::string getHeaderElement(unsigned int pos) const;
    const std::string& getFileName(void) const;

public:
    bool deleteRow(unsigned int row);
    bool addRow(unsigned int pos, const std::vector<std::string>&);
    void sync(void) const;

protected:
    void parseHeader(void);
    void parseContent(void);

private:
    std::string _file;
    const DataType _type;
    const char _sep;
    const bool _head;
    std::vector<std::string> _originalFile;
    std::vector<std::string> _header;
    std::vector<Row*> _content;

public:
    Row& operator[](unsigned int row) const;
};


//============================================================================
// Parser member functions
//============================================================================

Parser::Parser(const std::string& data, const DataType& type, char sep, bool head)
    : _type(type), _sep(sep), _head(head)
{
    std::string line;
    if (type == eFILE)
    {
        _file = data;
        std::ifstream ifile(_file.c_str());
        if (ifile.is_open())
        {
            while (ifile.good())
            {
                getline(ifile, line);
                if (line != "")
                    _originalFile.push_back(line);
            }
            ifile.close();

            if (_originalFile.size() == 0)
                throw Error(std::string("No Data in ").append(_file));

            if (_head == true) parseHeader();
            parseContent();
        }
        else
            throw Error(std::string("Failed to open ").append(_file));
    }
    else
    {
        std::istringstream stream(data);
        while (std::getline(stream, line))
            if (line != "")
                _originalFile.push_back(line);
        if (_originalFile.size() == 0)
            throw Error(std::string("No Data in pure content"));

        if (_head == true) parseHeader();
        parseContent();
    }
}

Parser::~Parser(void)
{
    std::vector<Row*>::iterator it;

    for (it = _content.begin(); it != _content.end(); it++)
        delete* it;
}

void Parser::parseHeader(void)
{
    std::stringstream ss(_originalFile[0]);
    std::string item;

    while (std::getline(ss, item, _sep))
        _header.push_back(item);
}

void Parser::parseContent(void)
{
    std::vector<std::string>::iterator it;

    it = _originalFile.begin();
    if (_head == true) it++; // skip header

    for (; it != _originalFile.end(); it++)
    {
        bool quoted = false;
        int tokenStart = 0;
        unsigned int i = 0;

        // calculate the number of columns for the row
        int columns = count(it->begin(), it->end(), ',') + 1;

        // build a fake header to satisfy the data structure
        vector<string> fakeHeader;
        for (unsigned int i = 0; i < columns; i++)
        {
            fakeHeader.push_back("");
        }

        Row* row = new Row(fakeHeader);

        for (; i != it->length(); i++)
        {
            if (it->at(i) == '"')
                quoted = ((quoted) ? (false) : (true));
            else if (it->at(i) == ',' && !quoted)
            {
                row->push(it->substr(tokenStart, i - tokenStart));
                tokenStart = i + 1;
            }
        }

        //end
        row->push(it->substr(tokenStart, it->length() - tokenStart));

        // if value(s) missing
        if (row->size() != (fakeHeader.size()))
            throw Error("corrupted data !");
        _content.push_back(row);
    }
}

Row& Parser::getRow(unsigned int rowPosition) const
{
    if (rowPosition < _content.size())
        return *(_content[rowPosition]);
    throw Error("can't return this row (doesn't exist)");
}

Row& Parser::operator[](unsigned int rowPosition) const
{
    return Parser::getRow(rowPosition);
}

unsigned int Parser::rowCount(void) const
{
    return _content.size();
}

unsigned int Parser::columnCount(void) const
{
    return _header.size();
}

std::vector<std::string> Parser::getHeader(void) const
{
    return _header;
}

const std::string Parser::getHeaderElement(unsigned int pos) const
{
    if (pos >= _header.size())
        throw Error("can't return this header (doesn't exist)");
    return _header[pos];
}

bool Parser::deleteRow(unsigned int pos)
{
    if (pos < _content.size())
    {
        delete* (_content.begin() + pos);
        _content.erase(_content.begin() + pos);
        return true;
    }
    return false;
}

bool Parser::addRow(unsigned int pos, const std::vector<std::string>& r)
{
    Row* row = new Row(_header);

    for (auto it = r.begin(); it != r.end(); it++)
        row->push(*it);

    if (pos <= _content.size())
    {
        _content.insert(_content.begin() + pos, row);
        return true;
    }
    return false;
}

void Parser::sync(void) const
{
    if (_type == DataType::eFILE)
    {
        std::ofstream f;
        f.open(_file, std::ios::out | std::ios::trunc);

        // header
        unsigned int i = 0;
        for (auto it = _header.begin(); it != _header.end(); it++)
        {
            f << *it;
            if (i < _header.size() - 1)
                f << ",";
            else
                f << std::endl;
            i++;
        }

        for (auto it = _content.begin(); it != _content.end(); it++)
            f << **it << std::endl;
        f.close();
    }
}

const std::string& Parser::getFileName(void) const
{
    return _file;
}


//============================================================================
// Binary Search Tree class definition
//============================================================================

/**
 * Define a class containing data members and methods to
 * implement a binary search tree
 */
class BinarySearchTree {

private:
    Node* root;

    void addNode(Node* node, Course course);
    void destroyNode(Node* node);
    void inOrder(Node* node);
    void validateCourses(Node* node);
    Node* removeNode(Node* node, string courseId);

public:
    BinarySearchTree();
    virtual ~BinarySearchTree();
    void InOrder();
    void ValidateCourses();
    void Insert(Course course);
    void Remove(string courseId);
    Course Search(string courseId);
};


//============================================================================
// Binary Search Tree member fucntions
//============================================================================

/**
 * Default constructor
 */
BinarySearchTree::BinarySearchTree() {
    // initialize root
    root = nullptr;
}

/**
 * Destructor
 */
BinarySearchTree::~BinarySearchTree() {
    // recurse from root deleting every node
    destroyNode(root);
}

void BinarySearchTree::destroyNode(Node* node) {
    // helper function for destructor
    // recursively delete each node in tree

    if (node != nullptr) {
        destroyNode(node->left);
        destroyNode(node->right);
        delete node;
    }
}

/**
 * Traverse the tree in order
 */
void BinarySearchTree::InOrder() {
    // print in order starting from root
    inOrder(root);
}

void BinarySearchTree::ValidateCourses() {
    // validate in order starting from root
    validateCourses(root);
}

/**
 * Insert a course
 */
void BinarySearchTree::Insert(Course course) {
    // special case for no root node
    if (root == nullptr) {
        root = new Node(course);
    }

    // else add node to tree
    else {
        addNode(root, course);
    }
}

/**
 * Remove a course
 */
void BinarySearchTree::Remove(string courseId) {
    // remove node matching courseId, begin search from root
    root = removeNode(root, courseId);
}

/**
 * Search for a course
 */
Course BinarySearchTree::Search(string courseId) {
    // if node = nullptr return node
    Node* node = root;

    while (node != nullptr) {
        // courseId matches, return course
        if (courseId.compare(node->course.courseId) == 0) {
            return node->course;
        }
        // target courseId greater than node courseId, search right
        else if (courseId.compare(node->course.courseId) > 0) {
            node = node->right;
        }
        // target courseId lesser than node courseId, search left
        else if (courseId.compare(node->course.courseId) < 0) {
            node = node->left;
        }
    }

    // courseId wasn't found, return empty course
    Course course;
    return course;
}

/**
 * Add a course to some node (recursive)
 *
 * @param node Current node in tree
 * @param Course course to be added
 */
void BinarySearchTree::addNode(Node* node, Course course) {
    // if node is larger then add to left
    //course.courseId.compare(node->course.courseId) < 0
    if (course.courseId.compare(node->course.courseId) < 0) {
        // if no left node this node becomes left node
        if (node->left == nullptr) {
            node->left = new Node(course);
        }
        // else recurse down the left tree
        else {
            this->addNode(node->left, course);
        }
    }
    // if node is smaller then add to right
    else {
        // if no right node this node becomes right node
        if (node->right == nullptr) {
            node->right = new Node(course);
        }
        // else recurse down right tree
        else {
            this->addNode(node->right, course);
        }
    }
}

void BinarySearchTree::inOrder(Node* node) {
    // if node exists
    if (node != nullptr) {
        // recurse left tree
        inOrder(node->left);

        // print current node details
        cout << node->course.courseId << ", "
            << node->course.courseName << endl;

        // recurse right tree
        inOrder(node->right);
    }
}

void BinarySearchTree::validateCourses(Node* node) {
    // if node exists
    if (node != nullptr) {
        // recurse left tree
        validateCourses(node->left);

        // complain if courseId is blank
        if (node->course.courseId.empty()) 
        {
            cout << "Found entry with blank courseID.  Please review input data." << endl;
        }
        // complain if course name is blank
        else if (node->course.courseName.empty()) 
        {
            cout << "Course name for course " << node->course.courseId
                << " is blank.  Please review input data." << endl;
        }

        // verify prerequisites exist
        bool valid = true;
        for (string preReq : node->course.preReqList) 
        {
            // search tree for course with same ID as prereq
            Course course = Search(preReq);
            if (course.courseId.empty())
            {
                valid = false;
                break;
            }
        }

        // Prereq is invalid, complain
        if (!valid) {
            cout << "Course " << node->course.courseId <<
                " contains invalid prerequisite course(s).  Please review input data." << endl;
        }
        
        // recurse right tree
        validateCourses(node->right);
    }
}

/**
 * Remove a course from some node (recursive)
 */
Node* BinarySearchTree::removeNode(Node* node, string courseId) {
    // if node = nullptr return node
    if (node == nullptr) {
        return node;
    }

    // target course less than node course, recurse left side of tree
    if (courseId.compare(node->course.courseId) < 0) {
        node->left = removeNode(node->left, courseId);
    }
    // target course greater than node course, recurse right side of tree
    else if (courseId.compare(node->course.courseId) > 0) {
        node->right = removeNode(node->right, courseId);
    }
    // target course matches
    else {
        // Node has no children, delete it
        if (node->left == nullptr && node->right == nullptr) {
            delete node;
            node = nullptr;
        }
        // Node has only left child
        else if (node->left != nullptr && node->right == nullptr) {
            Node* temp = node;
            node = node->left;
            delete temp;
        }
        // Node has only right child
        else if (node->left == nullptr && node->right != nullptr) {
            Node* temp = node;
            node = node->right;
            delete temp;
        }
        // node has both left and right children
        else {
            // find leftmost node from right subtree
            Node* temp = node->right;
            while (temp->left != nullptr) {
                temp = temp->left;
            }

            // replace course with course from successor node
            node->course = temp->course;

            // remove successor node from right subtree
            node->right = removeNode(node->right, temp->course.courseId);
        }
    }
    return node;
}



//============================================================================
// Static methods used for testing
//============================================================================

/**
 * Display the course information to the console (std::out)
 *
 * @param course struct containing the course info
 */
void displayCourse(Course course) {
    // print course details
    cout << course.courseId << ", "
        << course.courseName << endl;
        
    // if the course has prerequisites
    if (course.preReqList.size() > 0) {
        cout << "Prerequisites:" << endl;
        // build prereq list
        string prerequisites = "";
        for (string preReq : course.preReqList) {
            prerequisites += preReq + ", ";
        }

        // clear trailing comma
        prerequisites = prerequisites.substr(0, prerequisites.size() - 2);

        // print prereq list
        cout << prerequisites << endl;
    }

    cout << endl;

    return;
}

/**
 * Load a CSV file containing courses into a container
 *
 * @param csvPath the path to the CSV file to load
 * @return a container holding all the courses read
 */
void loadCourses(string csvPath, BinarySearchTree* bst) {
    cout << "Loading CSV file " << csvPath << endl;

    // initialize the CSV Parser using the given path
    Parser file = Parser(csvPath);

    // read and display header row - optional
    vector<string> header = file.getHeader();
    for (auto const& c : header) {
        cout << c << " | ";
    }
    cout << "" << endl;

    try {
        // loop to read rows of a CSV file
        for (unsigned int i = 0; i < file.rowCount(); i++) {

            // Create and populate data structure for course
            Course course;
            course.courseId = file[i][0];
            course.courseName = file[i][1];
            
            // Populate course preReqList based on number of columns
            for (unsigned int j = 2; j < file.getRow(i).size(); j++) {
                // only add non-blank entries
                if (file[i][j].length() > 0) course.preReqList.push_back(file[i][j]);
            }

            // add course to the BST
            bst->Insert(course);
        }
    }
    catch (Error& e) {
        std::cerr << e.what() << std::endl;
    }
}

/**
 * Simple C function to convert a string to a double
 * after stripping out unwanted char
 *
 * credit: http://stackoverflow.com/a/24875936
 *
 * @param ch The character to strip out
 */
double strToDouble(string str, char ch) {
    str.erase(remove(str.begin(), str.end(), ch), str.end());
    return atof(str.c_str());
}

/**
 * The one and only main() method
 */
int main(int argc, char* argv[]) {

    // process command line arguments
    string csvPath, courseKey;
    switch (argc) {
    case 2:
        csvPath = argv[1];
        courseKey = "98223";
        break;
    case 3:
        csvPath = argv[1];
        courseKey = argv[2];
        break;
    default:
        csvPath = "CS 300 ABCU_Advising_Program_Input.csv";
        courseKey = "CSCI400";
    }

    // Define a binary search tree to hold all courses
    BinarySearchTree* bst;
    bst = new BinarySearchTree();
    Course course;

    string input;
    int choice = 0;
    while (choice != 9) {
        cout << "Menu:" << endl;
        cout << "  1. Load Data Structure." << endl;
        cout << "  2. Print Course List." << endl;
        cout << "  3. Print Course." << endl;
        cout << "  9. Exit" << endl;
        cout << "Enter choice: ";

        // get input
        getline(cin, input);

        // attempt to convert input to int
        try
        {
            choice = atoi(&input.at(0));
            // zero means input was invalid
            if (choice == 0) 
            {
                cout << "Invalid command." << endl;
                continue;
            }
        }
        // failure means input was invalid
        catch (...)
        {
            cout << "Invalid command." << endl;
            continue;
        }

        switch (choice) {

        case 1:
            cout << endl << "Enter the path of the file you wish to load:" << endl;

            // get input file path
            getline(cin, csvPath);

            // Complete the method call to load the courses
            loadCourses(csvPath, bst);

            // Validate the load
            bst->ValidateCourses();

            break;

        case 2:
            // Complete method call to print all the courses
            cout << endl;
            bst->InOrder();
            cout << endl;
            break;

        case 3:
            cout << endl << "What course do you want to know about?" << endl;

            // get input course key
            getline(cin, courseKey);

            // Ensure course key is upper case
            transform(courseKey.begin(), courseKey.end(), courseKey.begin(), ::toupper);

            course = bst->Search(courseKey);

            if (!course.courseId.empty()) {
                displayCourse(course);
            }
            else {
                cout << "Course Id " << courseKey << " not found." << endl;
            }
            break;

        case 9:
            break;

        default:
            cout << choice << " is not a valid option." << endl;
            break;
        }
    }

    cout << "Thank you for using course planner!" << endl;

    return 0;
}
