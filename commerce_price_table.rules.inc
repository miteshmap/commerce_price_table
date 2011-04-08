<?php

/**
 * Implements hook_rules_action_info().
 */
function commerce_price_table_rules_action_info() {
  $actions = array();

  $actions['commerce_price_table_set_price'] = array(
    'label' => t('Replace the price for a price table'),
    'parameter' => array(
      'line_item' => array(
        'type' => 'commerce_line_item',
        'label' => t('Line item'),
      ),
    ),
    'group' => t('Commerce price table'),
  );

  return $actions;
}

function commerce_price_table_set_price($line_item) {
  // If the line item contains a product, we set the price according to the
  // quantity.
  if (commerce_line_items_quantity(array($line_item), 'product') > 0) {
    $line_item_wrapper = entity_metadata_wrapper('commerce_line_item', $line_item);
    $product = commerce_product_load($line_item_wrapper->commerce_product->product_id->value());
    $item = commerce_price_table_get_amount_qty($product, $line_item_wrapper->quantity->value());
    if (isset($item)) {
      $line_item_wrapper->commerce_unit_price->amount = $item['amount'];
      $base_price = $line_item_wrapper->commerce_unit_price->data->value();
      $base_price['components'][0]['price']['amount'] = $item['amount'];
      $line_item_wrapper->commerce_unit_price->data = $base_price;
    }
  }
}
