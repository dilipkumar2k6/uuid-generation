import uuid

class UuidV4Generator:
    def next_id_string(self):
        return str(uuid.uuid4())
