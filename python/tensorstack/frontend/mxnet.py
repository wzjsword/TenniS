#!/usr/bin/env python

"""
Author: Kier
"""

from .. import zoo
from .. import menu
from .. import Node
from .. import device


class Name(object):
    class Layer(object):
        pooling2d_padding = "_mx_pooling2d_padding"

    valid = "valid"


class Type(object):
    class padding_size(object):
        valid = 0
        same = 1


def pooling2d_padding(name, x, padding, ksize, stride, format=zoo.Name.NCHW, valid=False):
    assert isinstance(x, Node)

    # param
    padding = zoo.to_const(padding, "padding")

    # input
    ksize = zoo.to_node(ksize, name="_const_" + name + "_ksize", device=device.CPU)
    stride = zoo.to_node(stride, name="_const_" + name + "_ksize", device=device.CPU)

    # operator
    node = menu.op(name=name, op_name=Name.Layer.pooling2d_padding, inputs=[x, ksize, stride])
    node.set(zoo.Name.format, format)
    node.set(zoo.Name.padding, padding)
    node.set(Name.valid, valid)

    return node


def pooling2d(name, x, ksize, stride, type=zoo.Type.pooling_type.max, format=zoo.Name.NCHW,
              padding=None,
              padding_type=zoo.Type.padding_type.black,
              valid=False):
    assert isinstance(x, Node)

    # param
    static_padding = zoo.to_const(padding, "padding")

    if not valid:
        return zoo.pooling2d(name=name, x=x, ksize=ksize, stride=stride,
                             type=type, format=format, padding=static_padding, padding_type=padding_type)

    # input
    ksize = zoo.to_node(ksize, name="_const_" + name + "_ksize", device=device.CPU)
    stride = zoo.to_node(stride, name="_const_" + name + "_ksize", device=device.CPU)

    # operator
    dynamic_padding = pooling2d_padding(name="_op_" + name + "_valid_padding",
                                        x=x, padding=static_padding, ksize=ksize, stride=stride, format=format,
                                        valid=True)

    return zoo.pooling2d_v2(name=name, x=x, ksize=ksize, stride=stride,
                            type=type, format=format, padding=dynamic_padding, padding_type=padding_type)
