package com.uuidgenerator.lib.uuidv4;

import com.uuidgenerator.lib.IdGenerator;

import java.util.UUID;

public class UuidV4Generator implements IdGenerator {
    @Override
    public String nextIdString() {
        return UUID.randomUUID().toString();
    }
}
