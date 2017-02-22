#!/usr/bin/env python2.7
import tensorflow as tf
import tempfile
import tarfile
import os
import os.path
import sys


def save_model(sess, inp, out, out_dir):
    sign_def = tf.saved_model.signature_def_utils.build_signature_def(
        inputs={'inp': tf.saved_model.utils.build_tensor_info(inp)},
        outputs={'out': tf.saved_model.utils.build_tensor_info(out)},
        method_name=tf.saved_model.signature_constants.PREDICT_METHOD_NAME
    )
    builder = tf.saved_model.builder.SavedModelBuilder(out_dir)
    builder.add_meta_graph_and_variables(
        sess,
        [tf.saved_model.tag_constants.SERVING],
        signature_def_map={
            tf.saved_model.signature_constants.
            DEFAULT_SERVING_SIGNATURE_DEF_KEY:
                sign_def
        }
    )
    builder.save()


def pack_arch(in_dir, target_file, in_arch_base):
    with tarfile.open(target_file, "w") as arch:
        for root, _, files in os.walk(in_dir):
            for f in files:
                full_name = os.path.join(root, f)
                in_arch_name = os.path.join(
                    in_arch_base,
                    os.path.relpath(full_name, in_dir)
                )
                arch.add(full_name, in_arch_name)


def main():
    _, target_file = sys.argv

    models_dir = tempfile.mkdtemp()

    for model, k1v in [("a", 100), ("b", 200)]:
        for version, k2v in [("1", 10), ("2", 20)]:
            with tf.Session(graph=tf.Graph()) as sess:
                inp = tf.placeholder(tf.int32)
                # We have to make these variables, not constants, because
                # TensorFlow Serving expects at least some variables in the
                # model. See https://github.com/tensorflow/serving/issues/317.
                k1 = tf.Variable(0, name="k1", dtype=tf.int32)
                k2 = tf.Variable(0, name="k2", dtype=tf.int32)
                out = k1 + k2 + inp
                set1 = k1.assign(tf.constant(k1v))
                set2 = k2.assign(tf.constant(k2v))

                sess.run(tf.global_variables_initializer())
                sess.run(set1)
                sess.run(set2)

                save_path = os.path.join(models_dir, model, version)
                save_model(sess, inp, out, save_path)

    pack_arch(models_dir, target_file, in_arch_base="models")

if __name__ == "__main__":
    main()
