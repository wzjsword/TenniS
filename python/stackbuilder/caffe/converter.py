#!python
# coding: UTF-8
"""
author: kier
"""

from .proto import caffe_pb2 as caffe
from . import parser

import tensorstack as ts


def convert(prototxt, caffemodel, output_file):
    net = caffe.NetParameter()
    with open(prototxt, "rb") as file_prototxt:
        from google.protobuf import text_format
        binary = file_prototxt.read()
        text = binary.decode("utf-8")
        text_format.Parse(text, net)
    params = caffe.NetParameter()
    with open(caffemodel, "rb") as file_caffemodel:
        binary = file_caffemodel.read()
        params.ParseFromString(binary)

    layer2params = {}
    # load params
    for param_layer in params.layer:
        layer_params = param_layer.blobs
        layer2params[param_layer.name] = layer_params

    # load net structure
    layers = net.layer

    deploy_input_name = []
    deploy_input_shape = []

    if len(net.input) > 0 and len(net.input_shape) > 0:
        assert len(net.input) == len(net.input_shape)
        for i in range(len(net.input)):
            input_name = net.input[i]
            input_shape = parser.blob_shape(net.input_shape[i])
            deploy_input_name.append(input_name)
            deploy_input_shape.append(input_shape)

    # function format(layer, params, input_nodes, output_names)
    layer_converters = {
        # add layer converter here
    }

    blob2nodes = {}
    # save blob used in top
    blob2count = {}

    def add_blob_count(blob):
        if blob in blob2count:
            blob2count[blob] += 1
        else:
            blob2count[blob] = 1

    # calculate each top count
    for input_name in deploy_input_name:
        add_blob_count(input_name)
    # ------------------------
    for layer in layers:
        # layer_params = layer.blobs
        # print "{}: {}".format(layer.name, len(layer_params))
        for top in layer.top:
            add_blob_count(top)

    # for input layer
    for i in range(len(deploy_input_name)):
        top = deploy_input_name[i]
        shape = deploy_input_shape[i]
        node_name = None
        if blob2count[top] <= 1:
            node_name = top
        else:
            blob2count[top] -= 1
            node_name = "{}_hide_{}".format(top, blob2count[top])
        node = ts.menu.param("_origin_" + node_name, shape)
        node = ts.zoo.to_float(node_name, node)
        blob2nodes[top] = node

    # for each layer
    for layer in layers:
        # convert layer
        if layer.type not in layer_converters:
            raise Exception("Not supported Layer: {}".format(layer.type))
        ts_converter = layer_converters[layer.type]

        # gather input nodes
        input_nodes = []
        for bottom in layer.bottom:
            if bottom not in blob2nodes:
                raise Exception("Not computed blob: {}".format(bottom))
            input_nodes.append(blob2nodes[bottom])

        # set output names
        output_names = []
        for top in layer.top:
            if blob2count[top] <= 1:
                output_names.append(top)
            else:
                blob2count[top] -= 1
                output_names.append("{}_hide_{}".format(top, blob2count[top]))

        # query params
        params = []
        if layer.name in layer2params:
            params = layer2params[layer.name]

        ts_nodes = ts_converter(layer=layer, params=params, input_nodes=input_nodes, output_names=output_names)

        assert len(ts_nodes) == len(layer.top)

        for i in range(len(layer.top)):
            # update blob2nodes
            blob2nodes[layer.top[i]] = ts_nodes[i]

    print blob2count

    pass