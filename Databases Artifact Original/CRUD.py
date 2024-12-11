# Joe Clancy
# 6/3/2024
# CS340
# Project One

from pymongo import MongoClient


class AnimalShelter(object):
    """ CRUD operations for Animal collection in MongoDB """

    def __init__(self):
        # Initialize AnimalShelter object with MongoDB connection.

        # Initialize URI elements, ignore security best practices
        user = 'aacuser'
        pwd = 'barkwoof'
        host = 'nv-desktop-services.apporto.com'
        port = 31368
        db = 'AAC'
        col = 'animals'

        # Asemble connection URI
        uri = 'mongodb://%s:%s@%s:%d' % (user, pwd, host, port)

        # Establish MongoDB connection
        self.client = MongoClient(uri)
        self.db = self.client[db]
        self.col = self.db[col]

    def create_document(self, document: dict) -> bool:
        # Method to implement the C in CRUD.

        # Attempt to insert document.
        try:
            self.col.insert_one(document)
            return True

        # Handle error.
        except Exception as e:
            print(f"An error occurred while inserting the document: {e}")
            return False

    def read_documents(self, query: dict) -> list:
        # Method to implement the R in CRUD.

        # Attempt to retrieve document(s).
        try:
            cursor = self.col.find(query)
            return list(cursor)

        # Handle error.
        except Exception as e:
            print(f"An error occurred while querying the documents: {e}")
            # Because of error, no docs are returned.
            return []

    def update_document(self, query: dict, update: dict, update_many: bool = False) -> int:
        # Method to implement the U in CRUD.
        # If update_many is set true, function will use the update_many() API call.

        # Attempt to update document(s).
        try:
            # Use update_many() API call.
            if update_many:
                result = self.col.update_many(query, {'$set': update})
            # Use update_one() API call.
            else:
                result = self.col.update_one(query, {'$set': update})

            # Return number of docs modified.
            return result.modified_count

        # Handle error.
        except Exception as e:
            print(f"An error occurred while updating the document(s): {e}")
            # Because of error, no docs were modified.
            return 0

    def delete_document(self, query: dict, delete_many: bool = False) -> int:
        # Method to implement the D in CRUD.
        # if delete_many is set true, function will use the delete_many() API call.

        # Attempt to delete document(s).
        try:
            # Use delete_many() API call.
            if delete_many:
                result = self.col.delete_many(query)
            # Use delete_one() API call.
            else:
                result = self.col.delete_one(query)

            # Return number of docs deleted.
            return result.deleted_count

        # Handle error.
        except Exception as e:
            print(f"An error occurred while deleting the document(s): {e}")
            # Because of error, no docs were deleted.
            return 0