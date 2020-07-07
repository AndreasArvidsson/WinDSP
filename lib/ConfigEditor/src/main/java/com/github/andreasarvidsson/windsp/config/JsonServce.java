package com.github.andreasarvidsson.windsp.config;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import javax.ws.rs.Consumes;
import javax.ws.rs.GET;
import javax.ws.rs.PUT;
import javax.ws.rs.Path;
import javax.ws.rs.Produces;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;

/**
 *
 * @author Andreas Arvidsson
 */
@Path("json")
public class JsonServce {

    @GET
    @Produces(MediaType.APPLICATION_JSON)
    public Response get() {
        return Response.ok(ConfFile.get()).build();
    }

    @PUT
    @Consumes(MediaType.APPLICATION_JSON)
    public void save(final String str) throws IOException {
        final File file = ConfFile.get();
        try (final FileWriter writer = new FileWriter(file)) {
            writer.write(str);
        }
    }

}
